
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import asyncio
import time
import ncclient
import ncclient.asyncio_manager
import re

from gi.repository import (
    RwYang,
    RwNsmYang as nsmY,
    RwDts as rwdts,
    RwTypes,
    RwConmanYang as conmanY
)

import rift.tasklets

class ROSOConnectionError(Exception):
    pass

class ROServiceOrchif(object):

    def __init__(self, log, loop, parent):
        self._log = log
        self._loop = loop
        self._parent = parent
        self._manager = None
        try:
            self._model = RwYang.Model.create_libncx()
            self._model.load_schema_ypbc(nsmY.get_schema())
            self._model.load_schema_ypbc(conmanY.get_schema())
        except Exception as e:
            self._log.error("Error generating models %s", str(e))
            

    @asyncio.coroutine
    def connect(self):
        so_endp = self._parent.cm_endpoint
        try:
            self._log.info("Attemping Resource Orchestrator netconf connection.")
            self._manager = yield from ncclient.asyncio_manager.asyncio_connect(loop=self._loop,
                                                                                host=so_endp['cm_ip_address'],
                                                                                port=so_endp['cm_port'],
                                                                                username=so_endp['cm_username'],
                                                                                password=so_endp['cm_password'],
                                                                                allow_agent=False,
                                                                                look_for_keys=False,
                                                                                hostkey_verify=False)
            self._log.info("Connected to Service Orchestrator netconf @%s", so_endp['cm_ip_address'])
            return True
        except Exception as e:
            self._log.error("Netconf connection to Service Orchestrator ip %s failed: %s",
                            so_endp['cm_ip_address'], str(e))
            return False

    @staticmethod
    def wrap_netconf_config_xml(xml):
        xml = '<?xml version="1.0" encoding="UTF-8"?><config xmlns:xc="urn:ietf:params:xml:ns:netconf:base:1.0">{}</config>'.format(xml)
        return xml

    def send_nsr_update(self, nsrid):
        self._log.debug("Attempting to send NSR id: %s", nsrid)
        msg = conmanY.SoConfig()
        addnsr = msg.nsr.add()
        addnsr.id = nsrid
        xml = msg.to_xml_v2(self._model)
        netconf_xml = self.wrap_netconf_config_xml(xml)
        
        try:
            response = yield from self._manager.edit_config(target='running', config = netconf_xml)
            self._log.info("Received edit config response: %s", str(response))
        except ncclient.transport.errors.SSHError as e:
            so_endp = self._parent.cm_endpoint
            self._log.error("Applying configuration %s to SO(%s) failed: %s",
                            netconf_xml, so_endp['cm_ip_address'], str(e))
        return
        
    def send_nsr_delete(self, nsrid):
        self._log.debug("Attempting to send delete NSR id: %s", nsrid)
        msg = conmanY.SoConfig()
        addnsr = msg.nsr.add()
        addnsr.id = nsrid
        xml = msg.to_xml_v2(self._model)
        delete_path = '/cm-config/nsr[id=\'{}\']'.format(nsrid)

        def _xpath_strip_keys(xpath):
            ''' Copied from automation '''
            '''Strip key-value pairs from the supplied xpath

            Arguments:
                xpath - xpath to be stripped of keys

            Returns:
                an xpath without keys
            '''
            RE_CAPTURE_KEY_VALUE = re.compile(r'\[[^=]*?\=[\"\']?([^\'\"\]]*?)[\'\"]?\]')
            return re.sub(RE_CAPTURE_KEY_VALUE, '', xpath)

        # In leiu of protobuf delta support, try to place the attribute in the correct place
        def add_attribute(xpath, xml):
            xpath = xpath.lstrip('/')
            xpath = _xpath_strip_keys(xpath)
            xpath_elems = xpath.split('/')
            pos = 0
            for elem in xpath_elems:
                pos = xml.index(elem, pos)
                pos = xml.index('>', pos)
            if xml[pos-1] == '/':
                pos -= 1
            xml = xml[:pos] + " xc:operation='delete'" + xml[pos:]
            return xml

        xml = add_attribute(delete_path, xml)
        # print('>>>>> delete xml=\n{}\n\n'.format(xml))
        netconf_xml = '<config xmlns:xc="urn:ietf:params:xml:ns:netconf:base:1.0">{}</config>'.format(xml)

        try:
            response = yield from self._manager.edit_config(target='running', config = netconf_xml)
            self._log.info("Received delete config response: %s", str(response))
        except ncclient.transport.errors.SSHError as e:
            self._log.error("Deleting CM config for NSR id=%s failed: %s",
                                                        nsrid, str(e))
        return
        
class ROServiceOrchConfig(object):
    def __init__(self, log, loop, dts):
        self._log = log
        self._loop = loop
        self._dts = dts
        #self._parent = parent
        self._ro_config_xpath = "C,/ro-config/cm-endpoint"
        self.soif = None
        self._active_nsr = []
        self.cm_endpoint = {}
        self._log.debug("Initialized ROServiceOrchConfig, cm_endpoint = %s", self.cm_endpoint)

    def is_ready(self):
        return True
    
    @asyncio.coroutine
    def register(self):
        """ Register for Nsd cm-endpoint requests from dts """

        @asyncio.coroutine
        def initiate_connection():
            loop_cnt = 60
            #Create SO interface object
            self._log.debug("Inside initiate_connection routine")
            self.soif = ROServiceOrchif(self._log, self._loop, self)
            for i in range(loop_cnt):
                connect_status = yield from self.soif.connect()
                if connect_status:
                    self._log.debug("Successfully connected to netconf")
                    for nsrid in self._active_nsr:
                        self._log.debug("Sending nsr-id : %s to SO from pending list", nsrid)
                        yield from self.soif.send_nsr_update(nsrid)
                        self._active_nsr.pop(nsrid)
                        self._log.debug("Deleting nsr-id : %s from pending list", nsrid)
                    break
                else:
                    self._log.error("Can not connect to SO. Retrying!")
                    
                self._log.debug("Sleeping for 1 second in initiate_connection()")
                yield from asyncio.sleep(1, loop = self._loop)
            else:
                raise ROSOConnectionError("Failed to connect to Service Orchestrator within 60")
            return
        
        def on_apply(dts, acg, xact, action, scratch):
            """Apply the  configuration"""
            ro_config = nsmY.RoConfig()
            
            if xact.xact is None:
                # When RIFT first comes up, an INSTALL is called with the current config
                # Since confd doesn't actally persist data this never has any data so
                # skip this for now.
                self._log.debug("No xact handle.  Skipping apply config")
                return

            self._log.debug("Got nsr apply cfg (xact:%s) (action:%s) (cm_endpoint:%s)",
                            xact, action, self.cm_endpoint)

            # Verify that cm_endpoint is complete, we may get only default values if this is confd re-apply
            so_complete = True
            for field in ro_config.cm_endpoint.fields:
                if field not in self.cm_endpoint:
                    so_complete = False
                    
            # Create future for connect
            if so_complete is True and self.soif is None:
                asyncio.ensure_future(initiate_connection(), loop = self._loop)
                    
            return 
        
        @asyncio.coroutine
        def on_prepare(dts, acg, xact, xact_info, ks_path, msg):
            """ Prepare callback from DTS for ro-config """

            self._log.debug("ro-config received msg %s", msg)

            action = xact_info.handle.get_query_action()
            # Save msg as dictionary
            msg_dict = msg.as_dict()

            self._log.info("ro-config received msg %s action %s - dict = %s", msg, action, msg_dict)

            # Save configuration infomration
            # Might be able to save entire msg_dict
            for key, val in msg_dict.items():
                self.cm_endpoint[key] = val
            
            acg.handle.prepare_complete_ok(xact_info.handle)

        self._log.debug(
            "Registering for NSD config using xpath: %s",
            self._ro_config_xpath
            )

        acg_hdl = rift.tasklets.AppConfGroup.Handler(on_apply=on_apply)
        with self._dts.appconf_group_create(handler=acg_hdl) as acg:
            self._regh = acg.register(xpath=self._ro_config_xpath,
                                      flags=rwdts.Flag.SUBSCRIBER,
                                      on_prepare=on_prepare)

        
    @asyncio.coroutine
    def notify_nsr_up(self, nsrid):
        self._log.info("Notifying NSR id = %s!", nsrid)
        
        if self.soif is None:
            self._log.warning("No SO interface created yet! Buffering the nsr-id")
            self._active_nsr.append(nsrid)
        else:
            # Send NSR id as configuration
            try:
                yield from self.soif.send_nsr_update(nsrid)
            except Exception as e:
                self._log.error("Failed to send NSR id to SO", str(e))
        return
        

    @asyncio.coroutine
    def notify_nsr_down(self, nsrid):
        self._log.info("Notifying NSR id = %s DOWN!", nsrid)

        if self.soif is None:
            self._log.warning("No SO interface created yet! find and delete the nsr-id from queue")
        else:
            # Send NSR id as configuration
            try:
                yield from self.soif.send_nsr_delete(nsrid)
            except Exception as e:
                self._log.error("Failed to send NSR id to SO", str(e))
        return

