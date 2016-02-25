
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#


import ncclient
import ncclient.asyncio_manager
import tornado.httpclient as tornadoh
import asyncio.subprocess
import asyncio
import time
import sys
import os, stat

from gi.repository import (
    RwDts as rwdts,
    RwYang,
    RwConmanYang as conmanY,
    RwNsrYang as nsrY,
    RwVnfrYang as vnfrY,
)

import rift.tasklets

if sys.version_info < (3, 4, 4):
    asyncio.ensure_future = asyncio.async

class ConfigManagerROifConnectionError(Exception):
    pass
class ScriptError(Exception):
    pass

class ConfigManagerROif(object):

    def __init__(self, log, loop, parent):
        self._log = log
        self._loop = loop
        self._parent = parent
        self._manager = None

        try:
            self._model = RwYang.Model.create_libncx()
            self._model.load_schema_ypbc(nsrY.get_schema())
            self._model.load_schema_ypbc(vnfrY.get_schema())
        except Exception as e:
            self._log.error("Error generating models %s", str(e))

        self.ro_config = self._parent._config.ro_config

    @property
    def manager(self):
        if self._manager is None:
            raise

        return self._manager

    @asyncio.coroutine
    def connect(self, timeout_secs=60):
        ro_cfg = self.ro_config
        start_time = time.time()
        while (time.time() - start_time) < timeout_secs:

            try:
                self._log.info("Attemping Resource Orchestrator netconf connection.")

                self._manager = yield from ncclient.asyncio_manager.asyncio_connect(
                    loop=self._loop,
                    host=ro_cfg['ro_ip_address'],
                    port=ro_cfg['ro_port'],
                    username=ro_cfg['ro_username'],
                    password=ro_cfg['ro_password'],
                    allow_agent=False,
                    look_for_keys=False,
                    hostkey_verify=False,
                )
                self._log.info("Connected to Resource Orchestrator netconf")
                return

            except ncclient.transport.errors.SSHError as e:
                self._log.error("Netconf connection to Resource Orchestrator ip %s failed: %s",
                                  ro_cfg['ro_ip_address'], str(e))

            yield from asyncio.sleep(2, loop=self._loop)

        self._manager = None
        raise ConfigManagerROifConnectionError(
            "Failed to connect to Resource Orchestrator within %s seconds" % timeout_secs
        )

    @asyncio.coroutine
    def get_nsr(self, id):
        self._log.debug("get_nsr() locals: %s", locals())
        xpath = "/ns-instance-opdata/nsr[ns-instance-config-ref='{}']".format(id)
        #xpath = "/ns-instance-opdata/nsr"
        self._log.debug("Attempting to get NSR using xpath: %s", xpath)
        response = yield from self._manager.get(
                filter=('xpath', xpath),
                )
        response_xml = response.data_xml.decode()

        self._log.debug("Received NSR(%s) response: %s", id, str(response_xml))

        try:
            nsr = nsrY.YangData_Nsr_NsInstanceOpdata_Nsr()
            nsr.from_xml_v2(self._model, response_xml)
        except Exception as e:
            self._log.error("Failed to load nsr from xml e=%s", str(e))
            return

        self._log.debug("Deserialized NSR response: %s", nsr)

        return nsr.as_dict()

    @asyncio.coroutine
    def get_vnfr(self, id):
        xpath = "/vnfr-catalog/vnfr[id='{}']".format(id)
        self._log.info("Attempting to get VNFR using xpath: %s", xpath)
        response = yield from self._manager.get(
                filter=('xpath', xpath),
                )
        response_xml = response.data_xml.decode()

        self._log.debug("Received VNFR(%s) response: %s", id, str(response_xml))

        vnfr = vnfrY.YangData_Vnfr_VnfrCatalog_Vnfr()
        vnfr.from_xml_v2(self._model, response_xml)

        self._log.debug("Deserialized VNFR response: %s", vnfr)

        return vnfr.as_dict()

class ConfigManagerEvents(object):
    def __init__(self, dts, log, loop, parent):
        self._dts = dts
        self._log = log
        self._loop = loop
        self._parent = parent
        self._nsr_xpath = "/cm-state/cm-nsr"

    def register(self):
        try:
            self._orif = ConfigManagerROif(self._log, self._loop, self._parent)
            self.register_cm_rpc()
        except Exception as e:
            self._log.debug("Failed to register (%s)", e)


    def register_cm_rpc(self):

        try:
            self._rpc_hdl = self._dts.register(
                xpath=self._nsr_xpath,
                handler=rift.tasklets.DTS.RegistrationHandler(
                    on_prepare=self.prepare_update_nsr),
                flags=rwdts.Flag.PUBLISHER)
        except Exception as e:
            self._log.debug("Failed to register xpath(%s) as (%s)", self._nsr_xpath, e)

    @asyncio.coroutine
    def prepare_update_nsr(self, xact_info, action, ks_path, msg):
        """ Prepare callback for the RPC """
        self._log("Received prepare_update_nsr with action=%s, msg=%s", action, msg)

        # Fetch VNFR for each VNFR id in NSR

    @asyncio.coroutine
    def apply_vnf_config(self, vnf_cfg):
        vnf_cfg['cm_state'] = conmanY.RecordState.CFG_DELAY
        yield from asyncio.sleep(vnf_cfg['config_delay'], loop=self._loop)
        vnf_cfg['cm_state'] = conmanY.RecordState.CFG_SEND
        try:
            if vnf_cfg['config_method'] == 'netconf':
                self._log.info("Creating ncc handle for VNF cfg = %s!", vnf_cfg)
                self.ncc = ConfigManagerVNFnetconf(self._log, self._loop, self._parent, vnf_cfg)
                if vnf_cfg['protocol'] == 'ssh':
                    yield from self.ncc.connect_ssh()
                else:
                    yield from self.ncc.connect()
                yield from self.ncc.apply_edit_cfg()
            elif vnf_cfg['config_method'] == 'rest':
                if self.rcc is None:
                    self._log.info("Creating rcc handle for VNF cfg = %s!", vnf_cfg)
                    self.rcc = ConfigManagerVNFrestconf(self._log, self._loop, self._parent, vnf_cfg)
                self.ncc.apply_edit_cfg()
            elif vnf_cfg['config_method'] == 'script':
                self._log.info("Executing script for VNF cfg = %s!", vnf_cfg)
                scriptc = ConfigManagerVNFscriptconf(self._log, self._loop, self._parent, vnf_cfg)
                yield from scriptc.apply_edit_cfg()
            elif vnf_cfg['config_method'] == 'juju':
                self._log.info("Executing juju config for VNF cfg = %s!", vnf_cfg)
                jujuc = ConfigManagerVNFjujuconf(self._log, self._loop, self._parent, vnf_cfg)
                yield from jujuc.apply_edit_cfg()
            else:
                self._log.error("Unknown configuration method(%s) received for %s",
                                vnf_cfg['config_method'], vnf_cfg['vnf_unique_name'])
                vnf_cfg['cm_state'] = conmanY.RecordState.CFG_FAILED
                return

            self._log.critical("Successfully applied configuration to (%s/%s_%d)",
                               vnf_cfg['nsr_name'],
                               vnf_cfg['vnfr_name'], vnf_cfg['member_vnf_index'])
        except Exception as e:
            self._log.error("Applying configuration(%s) file(%s) to (%s/%s_%d) at %s failed as: %s",
                            vnf_cfg['config_method'],
                            vnf_cfg['nsr_name'],
                            vnf_cfg['cfg_file'],
                            vnf_cfg['vnfr_name'], vnf_cfg['member_vnf_index'],
                            vnf_cfg['mgmt_ip_address'],
                            str(e))
            raise

class ConfigManagerVNFscriptconf(object):

    def __init__(self, log, loop, parent, vnf_cfg):
        self._log = log
        self._loop = loop
        self._parent = parent
        self._manager = None
        self._vnf_cfg = vnf_cfg

    #@asyncio.coroutine
    def apply_edit_cfg(self):
        vnf_cfg = self._vnf_cfg
        self._log.debug("Attempting to apply scriptconf to VNF: %s", vnf_cfg['mgmt_ip_address'])
        try:
            st = os.stat(vnf_cfg['cfg_file'])
            os.chmod(vnf_cfg['cfg_file'], st.st_mode | stat.S_IEXEC)
            #script_msg = subprocess.check_output(vnf_cfg['cfg_file'], shell=True).decode('utf-8')

            proc = yield from asyncio.create_subprocess_exec(
                vnf_cfg['script_type'], vnf_cfg['cfg_file'],
                stdout=asyncio.subprocess.PIPE)
            script_msg = yield from proc.stdout.read()
            rc = yield from proc.wait()

            if rc != 0:
                raise ScriptError(
                    "script config returned error code : %s" % rc
                    )

            self._log.debug("config script output (%s)", script_msg)
        except Exception as e:
            self._log.error("Error (%s) while executing script config", str(e))
            raise

class ConfigManagerVNFrestconf(object):

    def __init__(self, log, loop, parent, vnf_cfg):
        self._log = log
        self._loop = loop
        self._parent = parent
        self._manager = None
        self._vnf_cfg = vnf_cfg

    def fetch_handle(self, response):
        if response.error:
            self._log.error("Failed to send HTTP config request - %s", response.error)
        else:
            self._log.debug("Sent HTTP config request - %s", response.body)

    @asyncio.coroutine
    def apply_edit_cfg(self):
        vnf_cfg = self._vnf_cfg
        self._log.debug("Attempting to apply restconf to VNF: %s", vnf_cfg['mgmt_ip_address'])
        try:
            http_c = tornadoh.AsyncHTTPClient()
            # TBD
            # Read the config entity from file?
            # Convert connectoin-point?
            http_c.fetch("http://", self.fetch_handle)
        except Exception as e:
            self._log.error("Error (%s) while applying HTTP config", str(e))

class ConfigManagerVNFnetconf(object):

    def __init__(self, log, loop, parent, vnf_cfg):
        self._log = log
        self._loop = loop
        self._parent = parent
        self._manager = None
        self._vnf_cfg = vnf_cfg

        self._model = RwYang.Model.create_libncx()
        self._model.load_schema_ypbc(conmanY.get_schema())

    @asyncio.coroutine
    def connect(self, timeout_secs=120):
        vnf_cfg = self._vnf_cfg
        start_time = time.time()
        self._log.debug("connecting netconf .... %s", vnf_cfg)
        while (time.time() - start_time) < timeout_secs:

            try:
                self._log.info("Attemping VNF netconf connection.")

                self._manager = yield from ncclient.asyncio_manager.asyncio_connect(
                    loop=self._loop,
                    host=vnf_cfg['mgmt_ip_address'],
                    port=vnf_cfg['port'],
                    username=vnf_cfg['username'],
                    password=vnf_cfg['password'],
                    allow_agent=False,
                    look_for_keys=False,
                    hostkey_verify=False,
                )

                self._log.info("VNF netconf connected.")
                return

            except ncclient.transport.errors.SSHError as e:
                vnf_cfg['cm_state'] = conmanY.RecordState.FAILED_CONNECTION
                self._log.error("Netconf connection to VNF ip %s failed: %s",
                                vnf_cfg['mgmt_ip_address'], str(e))

            yield from asyncio.sleep(2, loop=self._loop)

        raise ConfigManagerROifConnectionError(
            "Failed to connect to VNF %s within %s seconds" %
            (vnf_cfg['mgmt_ip_address'], timeout_secs)
        )

    @asyncio.coroutine
    def connect_ssh(self, timeout_secs=120):
        vnf_cfg = self._vnf_cfg
        start_time = time.time()

        if (self._manager != None and self._manager.connected == True):
            self._log.debug("Disconnecting previous session")
            self._manager.close_session

        self._log.debug("connecting netconf via SSH .... %s", vnf_cfg)
        while (time.time() - start_time) < timeout_secs:

            try:
                vnf_cfg['cm_state'] = conmanY.RecordState.CONNECTING
                self._log.debug("Attemping VNF netconf connection.")

                self._manager = ncclient.asyncio_manager.manager.connect_ssh(
                    host=vnf_cfg['mgmt_ip_address'],
                    port=vnf_cfg['port'],
                    username=vnf_cfg['username'],
                    password=vnf_cfg['password'],
                    allow_agent=False,
                    look_for_keys=False,
                    hostkey_verify=False,
                )

                vnf_cfg['cm_state'] = conmanY.RecordState.NETCONF_SSH_CONNECTED
                self._log.debug("VNF netconf over SSH connected.")
                return

            except ncclient.transport.errors.SSHError as e:
                vnf_cfg['cm_state'] = conmanY.RecordState.FAILED_CONNECTION
                self._log.error("Netconf connection to VNF ip %s failed: %s",
                                vnf_cfg['mgmt_ip_address'], str(e))

            yield from asyncio.sleep(2, loop=self._loop)

        raise ConfigManagerROifConnectionError(
            "Failed to connect to VNF %s within %s seconds" %
            (vnf_cfg['mgmt_ip_address'], timeout_secs)
        )

    @asyncio.coroutine
    def apply_edit_cfg(self):
        vnf_cfg = self._vnf_cfg
        self._log.debug("Attempting to apply netconf to VNF: %s", vnf_cfg['mgmt_ip_address'])

        if self._manager is None:
            self._log.error("Netconf is not connected to %s, aborting!", vnf_cfg['mgmt_ip_address'])
            return

        # Get config file contents
        try:
            with open(vnf_cfg['cfg_file']) as f:
                configuration = f.read()
        except Exception as e:
            self._log.error("Reading contents of the configuration file(%s) failed: %s", vnf_cfg['cfg_file'], str(e))
            return

        # if self._parent.cfg_sleep:
        #     self._log.debug("apply_edit_cfg Sleeping now ... %s", vnf_cfg['mgmt_ip_address'])
        #     yield from asyncio.sleep(120, loop=self._loop)
        #     self._parent.cfg_sleep = False
        try:
            self._log.debug("apply_edit_cfg Woke up ... %s", vnf_cfg['mgmt_ip_address'])
            xml = '<config xmlns:xc="urn:ietf:params:xml:ns:netconf:base:1.0">{}</config>'.format(configuration)
            response = yield from self._manager.edit_config(xml, target='running')
            if hasattr(response, 'xml'):
                response_xml = response.xml
            else:
                response_xml = response.data_xml.decode()

            self._log.debug("apply_edit_cfg response: %s", response_xml)
            if '<rpc-error>' in response_xml:
                raise ConfigManagerROifConnectionError("apply_edit_cfg response has rpc-error : %s",
                                                       response_xml)

            self._log.debug("apply_edit_cfg Successfully applied configuration {%s}", xml)
        except:
            raise

class ConfigManagerVNFjujuconf(object):

    def __init__(self, log, loop, parent, vnf_cfg):
        self._log = log
        self._loop = loop
        self._parent = parent
        self._manager = None
        self._vnf_cfg = vnf_cfg

    #@asyncio.coroutine
    def apply_edit_cfg(self):
        vnf_cfg = self._vnf_cfg
        self._log.debug("Attempting to apply juju conf to VNF: %s", vnf_cfg['mgmt_ip_address'])
        try:
            args = ['python3',
                vnf_cfg['juju_script'],
                '--server', vnf_cfg['mgmt_ip_address'],
                '--user',  vnf_cfg['user'],
                '--password', vnf_cfg['secret'],
                '--port', str(vnf_cfg['port']),
                vnf_cfg['cfg_file']]
            self._log.error("juju script command (%s)", args)

            proc = yield from asyncio.create_subprocess_exec(
                *args,
                stdout=asyncio.subprocess.PIPE)
            juju_msg = yield from proc.stdout.read()
            rc = yield from proc.wait()

            if rc != 0:
                raise ScriptError(
                    "Juju config returned error code : %s" % rc
                    )

            self._log.debug("Juju config output (%s)", juju_msg)
        except Exception as e:
            self._log.error("Error (%s) while executing juju config", str(e))
            raise
