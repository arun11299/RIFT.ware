
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import asyncio

from gi.repository import (
    RwDts as rwdts,
    IetfNetworkYang,
    IetfNetworkTopologyYang,
    IetfL2TopologyYang,
    RwTopologyYang,
    RwsdnYang,
    RwTypes
)

from gi.repository.RwTypes import RwStatus
import rw_peas
import rift.tasklets

class SdnGetPluginError(Exception):
    """ Error while fetching SDN plugin """
    pass
  
  
class SdnGetInterfaceError(Exception):
    """ Error while fetching SDN interface"""
    pass


class SdnAccountMgr(object):
    """ Implements the interface to backend plugins to fetch topology """
    def __init__(self, log, log_hdl, loop):
        self._account = {}
        self._log = log
        self._log_hdl = log_hdl
        self._loop = loop
        self._sdn = {}

        self._regh = None

    def set_sdn_account(self,account):
        if (account.name in self._account):
            self._log.error("SDN Account is already set")
        else:
            sdn_account           = RwsdnYang.SDNAccount()
            sdn_account.from_dict(account.as_dict())
            sdn_account.name = account.name
            self._account[account.name] = sdn_account
            self._log.debug("Account set is %s , %s",type(self._account), self._account)
          

    def get_sdn_account(self, name):
        """
        Creates an object for class RwsdnYang.SdnAccount()
        """
        if (name in self._account):
            return self._account[name]
        else:
            self._log.error("ERROR : SDN account is not configured") 


    def get_sdn_plugin(self,name):
        """
        Loads rw.sdn plugin via libpeas
        """
        if (name in self._sdn):
            return self._sdn[name]
        account = self.get_sdn_account(name)
        plugin_name = getattr(account, account.account_type).plugin_name
        self._log.info("SDN plugin being created")
        plugin = rw_peas.PeasPlugin(plugin_name, 'RwSdn-1.0')
        engine, info, extension = plugin()

        self._sdn[name] = plugin.get_interface("Topology")
        try:
            rc = self._sdn[name].init(self._log_hdl)
            assert rc == RwStatus.SUCCESS
        except:
            self._log.error("ERROR:SDN plugin instantiation failed ")
        else:
            self._log.info("SDN plugin successfully instantiated")
        return self._sdn[name]


class NwtopDiscoveryDtsHandler(object):
    """ Handles DTS interactions for the Discovered Topology registration """
    DISC_XPATH = "D,/nd:network"

    def __init__(self, dts, log, loop, acctmgr, nwdatastore):
        self._dts = dts
        self._log = log
        self._loop = loop
        self._acctmgr = acctmgr
        self._nwdatastore = nwdatastore

        self._regh = None

    @property
    def regh(self):
        """ The registration handle associated with this Handler"""
        return self._regh

    @asyncio.coroutine
    def register(self):
        """ Register for the Discovered Topology path """

        @asyncio.coroutine
        def on_ready(regh, status):
            """  On_ready for Discovered Topology registration """
            self._log.debug("PUB reg ready for Discovered Topology handler regn_hdl(%s) status %s",
                                         regh, status)

        @asyncio.coroutine
        def on_prepare(xact_info, action, ks_path, msg):
            """ prepare for Discovered Topology registration"""
            self._log.debug(
                "Got topology on_prepare callback (xact_info: %s, action: %s): %s",
                xact_info, action, msg
                )

            if action == rwdts.QueryAction.READ:
                
                for name in self._acctmgr._account:
                    _sdnacct = self._acctmgr.get_sdn_account(name)
                    if (_sdnacct is None):
                        raise SdnGetPluginError

                    _sdnplugin = self._acctmgr.get_sdn_plugin(name)
                    if (_sdnplugin is None):
                        raise SdnGetInterfaceError

                    rc, nwtop = _sdnplugin.get_network_list(_sdnacct)
                    #assert rc == RwStatus.SUCCESS
                    if rc != RwStatus.SUCCESS:
                        self._log.error("Fetching get network list for SDN Account %s failed", name)
                        xact_info.respond_xpath(rwdts.XactRspCode.NACK)
                        return
                    
                    self._log.debug("Topology: Retrieved network attributes ")
                    for nw in nwtop.network:
                        # Add SDN account name
                        nw.rw_network_attributes.sdn_account_name = name
                        nw.network_id = name + ':' + nw.network_id
                        self._log.debug("...Network id %s", nw.network_id)
                        nw_xpath = ("D,/nd:network[network-id=\'{}\']").format(nw.network_id)
                        xact_info.respond_xpath(rwdts.XactRspCode.MORE,
                                        nw_xpath, nw)
                xact_info.respond_xpath(rwdts.XactRspCode.ACK)

                return
            else:
                err = "%s action on discovered Topology not supported" % action
                raise NotImplementedError(err)

        self._log.debug("Registering for discovered topology using xpath %s", NwtopDiscoveryDtsHandler.DISC_XPATH)

        handler = rift.tasklets.DTS.RegistrationHandler(
            on_ready=on_ready,
            on_prepare=on_prepare,
            )

        yield from self._dts.register(
            NwtopDiscoveryDtsHandler.DISC_XPATH,
            flags=rwdts.Flag.PUBLISHER,
            handler=handler
            )


class NwtopStaticDtsHandler(object):
    """ Handles DTS interactions for the Static Topology registration """
    STATIC_XPATH = "C,/nd:network"

    def __init__(self, dts, log, loop, acctmgr, nwdatastore):
        self._dts = dts
        self._log = log
        self._loop = loop
        self._acctmgr = acctmgr

        self._regh = None
        self.pending = {}
        self._nwdatastore = nwdatastore

    @property
    def regh(self):
        """ The registration handle associated with this Handler"""
        return self._regh
 
    
    @asyncio.coroutine
    def register(self):
        """ Register for the Static Topology path """

        @asyncio.coroutine
        def prepare_nw_cfg(dts, acg, xact, xact_info, ksp, msg):
            """Prepare for application configuration. Stash the pending
            configuration object for subsequent transaction phases"""
            self._log.debug("Prepare Network config received network id %s, msg %s",
                           msg.network_id, msg)
            self.pending[xact.id] = msg
            xact_info.respond_xpath(rwdts.XactRspCode.ACK)

        def apply_nw_config(dts, acg, xact, action, scratch):
            """Apply the pending configuration object"""
            if action == rwdts.AppconfAction.INSTALL and xact.id is None:
                self._log.debug("No xact handle.  Skipping apply config")
                return

            if xact.id not in self.pending:
                raise KeyError("No stashed configuration found with transaction id [{}]".format(xact.id))

            try:
                if action == rwdts.AppconfAction.INSTALL:
                    self._nwdatastore.create_network(self.pending[xact.id].network_id, self.pending[xact.id])
                elif action == rwdts.AppconfAction.RECONCILE:
                    self._nwdatastore.update_network(self.pending[xact.id].network_id, self.pending[xact.id])
            except:
                raise 

            self._log.debug("Create network config done")
            return RwTypes.RwStatus.SUCCESS

        self._log.debug("Registering for static topology using xpath %s", NwtopStaticDtsHandler.STATIC_XPATH)
        handler=rift.tasklets.AppConfGroup.Handler(
                        on_apply=apply_nw_config)

        with self._dts.appconf_group_create(handler=handler) as acg:
            acg.register(xpath = NwtopStaticDtsHandler.STATIC_XPATH, 
                                   flags = rwdts.Flag.SUBSCRIBER, 
                                   on_prepare=prepare_nw_cfg)


