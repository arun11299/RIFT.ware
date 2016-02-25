
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import asyncio
import sys
import subprocess
import yaml

import os

from gi.repository import (
    RwDts as rwdts,
    RwConmanYang as conmanY,
    ProtobufC,
)

import rift.tasklets

if sys.version_info < (3, 4, 4):
    asyncio.ensure_future = asyncio.async

def get_vnf_unique_name(nsr_name, vnfr_short_name, member_vnf_index):
    return "{}.{}.{}".format(nsr_name, vnfr_short_name, member_vnf_index)

class ConfigManagerConfig(object):
    def __init__(self, dts, log, loop, parent):
        self._dts = dts
        self._log = log
        self._loop = loop
        self._parent = parent
        self._res_config = {}
        self._nsr_dict = {}
        self.pending_cfg = {}
        self.terminate_cfg = {}
        self._config_xpath = "C,/cm-config"
        self._opdata_xpath = "D,/rw-conman:cm-state"

        self.cm_config = conmanY.SoConfig()
        # RO specific configuration
        self.ro_config = {}
        for key in self.cm_config.ro_endpoint.fields:
            self.ro_config[key] = None
        self.cm_states = "Initialized"

    @asyncio.coroutine
    def register(self):
        self.register_cm_config()
        yield from self.register_cm_opdata()

    def register_cm_config(self):
        def on_apply(dts, acg, xact, action, scratch):
            """Apply the Service Orchestration configuration"""
            if xact.id is None:
                # When RIFT first comes up, an INSTALL is called with the current config
                # Since confd doesn't actally persist data this never has any data so
                # skip this for now.
                self._log.debug("No xact handle.  Skipping apply config")
                return

            self._log.debug("cm-config (xact: %s) (action: %s)",
                            xact, action)

            if xact.id in self.terminate_cfg:
                msg = self.terminate_cfg.pop(xact.id, None)
                if msg is not None:
                    msg_dict = msg.as_dict()
                    for delnsr in msg_dict['nsr']:
                        nsr_id = delnsr.get('id', None)
                        asyncio.ensure_future(self.terminate_NSR(nsr_id), loop=self._loop)
                return
                
            if xact.id not in self.pending_cfg:
                self._log.debug("Could not find transaction data for transaction-id")
                return

            # Pop saved config (from on_prepare)
            msg = self.pending_cfg.pop(xact.id, None)
            self._log.debug("Apply cm-config: %s", msg)
            self.cm_states += ", cm-config"

            # Process entire configuration
            ro_cfg = self.ro_config

            msg_dict = msg.as_dict()
            self._log.debug("msg_dict is %s: %s", type(msg_dict), msg_dict)
            ''' Process Resource Orchestrator endpoint config '''
            if 'ro_endpoint' in msg_dict:
                self._log.debug("ro-endpoint = %s", msg_dict['ro_endpoint'])
                for key, value in msg_dict['ro_endpoint'].items():
                    ro_cfg[key] = value
                    self._log.debug("ro-config: key=%s, value=%s", key, ro_cfg[key])

                # If all RO credentials are configured, initiate connection

                ro_complete = True
                for key, value in ro_cfg.items():
                    if value is None:
                        ro_complete = False
                        self._log.warning("key %s is not set", key)
                # Get the ncclient handle (OR interface)
                orif = self._parent._event._orif
                # Get netconf connection
                if ro_complete is True and orif._manager is None:
                    self._log.info("Connecting to RO = %s!", ro_cfg['ro_ip_address'])
                    asyncio.wait(asyncio.ensure_future(orif.connect(), loop=self._loop))
                    #asyncio.ensure_future(orif.connect(), loop=self._loop)
                    self._log.info("Connected to RO = %s!", ro_cfg['ro_ip_address'])
                    self.cm_states += ", RO connected"
                else:
                    self._log.warning("Already connected to RO, ignored!")

            if 'nsr' in msg_dict:
                for addnsr in msg_dict['nsr']:
                    ''' Process Initiate NSR '''
                    nsr_id = addnsr.get('id', None)
                    if nsr_id != None:
                        asyncio.ensure_future(self.config_NSR(nsr_id), loop=self._loop)

            return

        @asyncio.coroutine
        def on_prepare(dts, acg, xact, xact_info, ks_path, msg):
            action = xact_info.handle.get_query_action()

            self._log.debug("Received cm-config: xact.id=%s, msg=%s, action=%s", xact.id, msg, action)
            # print("<1<<< self.pending_cfg:", self.pending_cfg)
            # fref = ProtobufC.FieldReference.alloc()
            # pb_msg = msg.to_pbcm()
            # fref.goto_whole_message(pb_msg)
            # print(">>>>>> fref.is_field_deleted():", fref.is_field_deleted())

            msg_dict = msg.as_dict()
            pending_q = self.pending_cfg
            if action == rwdts.QueryAction.DELETE:
                pending_q = self.terminate_cfg
                if 'nsr' in msg_dict:
                    # Do this only if NSR is deleted
                    # fref = ProtobufC.FieldReference.alloc()
                    # pb_msg = msg.to_pbcm()
                    # fref.goto_whole_message(pb_msg)
                    # print(">>>>>> fref.is_field_deleted():", fref.is_field_deleted())
                    # # Got DELETE action in prepare callback
                    # if fref.is_field_deleted():

                    # NS is(are) terminated
                    for delnsr in msg_dict['nsr']:
                        nsr_id = delnsr.get('id', None)
                        # print('>>>>>>> Will delete pending NSR id={}'.format(nsr_id))
                        if nsr_id is not None:
                            # print(">>>>>>> self.pending_cfg:", self.pending_cfg)
                            # Find this NSR id if it is scheduled to be added.
                            for i,pending in self.pending_cfg.items():
                                p_dict = pending.as_dict()
                                if 'nsr' in p_dict:
                                    for p_nsr in p_dict['nsr']:
                                        p_nsr_id = p_nsr.get('id', None)
                                        if p_nsr_id == nsr_id:
                                            # Found it, remove
                                            self.pending_cfg.pop(i, None)
                                            pending_q = None

            # Enqueue the msg in proper queue
            if pending_q is not None:
                pending_q[xact.id] = msg
            acg.handle.prepare_complete_ok(xact_info.handle)

        self._log.debug("Registering for ro-config using xpath: %s",
                        self._config_xpath)

        acg_handler = rift.tasklets.AppConfGroup.Handler(on_apply = on_apply)

        with self._dts.appconf_group_create(handler=acg_handler) as acg:
            try:
                self._pool_reg = acg.register(xpath=self._config_xpath,
                                              flags=rwdts.Flag.SUBSCRIBER | rwdts.Flag.DELTA_READY,
                                              on_prepare=on_prepare)
                self._log.info("Successfully registered (%s)", self._config_xpath)
            except Exception as e:
                self._log.error("Failed to register as (%s)", e)


    @asyncio.coroutine
    def register_cm_opdata(self):

        def state_to_string(state):
            state_dict = {
                conmanY.RecordState.INIT : "init",
                conmanY.RecordState.RECEIVED : "received",
                conmanY.RecordState.CFG_PROCESS : "cfg_process",
                conmanY.RecordState.CFG_PROCESS_FAILED : "cfg_process_failed",
                conmanY.RecordState.CFG_SCHED : "cfg_sched",
                conmanY.RecordState.CFG_DELAY : "cfg_delay",
                conmanY.RecordState.CONNECTING : "connecting",
                conmanY.RecordState.FAILED_CONNECTION : "failed-connection",
                conmanY.RecordState.NETCONF_CONNECTED : "netconf-connected",
                conmanY.RecordState.NETCONF_SSH_CONNECTED : "netconf-ssh-connected",
                conmanY.RecordState.RESTCONF_CONNECTED : "restconf-connected",
                conmanY.RecordState.CFG_SEND : "cfg_send",
                conmanY.RecordState.CFG_FAILED : "cfg_failed",
                conmanY.RecordState.READY_NO_CFG : "ready_no_cfg",
                conmanY.RecordState.READY : "ready",
                }
            return state_dict[state]

        def prepare_show_output():
            self.show_output = conmanY.CmOpdata()
            self.show_output.states = self.cm_states
            nsr_dict = self._nsr_dict

            for nsr_obj in nsr_dict.values():
                cm_nsr = self.show_output.cm_nsr.add()
                # Fill in this NSR from nsr object
                cm_nsr.id = nsr_obj._nsr_id
                cm_nsr.state = state_to_string(nsr_obj.state)
                if nsr_obj.state == conmanY.RecordState.CFG_PROCESS_FAILED:
                    continue
                cm_nsr.name = nsr_obj.nsr_name

                # Fill in each VNFR from this nsr object
                vnfr_list = nsr_obj._vnfr_list
                for vnfr in vnfr_list:
                    vnf_cfg = vnfr['vnf_cfg']

                    # Create & fill vnfr
                    cm_vnfr = cm_nsr.cm_vnfr.add()
                    cm_vnfr.id = vnfr['id']
                    cm_vnfr.name = vnfr['name']
                    cm_vnfr.state = state_to_string(vnf_cfg['cm_state'])

                    # Fill in VNF management interface
                    cm_vnfr.mgmt_interface.ip_address = vnf_cfg['mgmt_ip_address']
                    cm_vnfr.mgmt_interface.cfg_type = vnf_cfg['config_method']
                    cm_vnfr.mgmt_interface.port = vnf_cfg['port']

                    # Fill in VNF configuration details
                    cm_vnfr.cfg_location = vnf_cfg['cfg_file']

                    # Fill in each connection-point for this VNF
                    cp_list = vnfr['connection_point']
                    for cp_item_dict in cp_list:
                        cm_cp = cm_vnfr.connection_point.add()
                        cm_cp.name = cp_item_dict['name']
                        cm_cp.ip_address = cp_item_dict['ip_address']


        @asyncio.coroutine
        def on_prepare(xact_info, action, ks_path, msg):
            self._log.debug("ConfigManager opdata response generation")

            if action == rwdts.QueryAction.CREATE:
                id = msg.id # TBD - get correct id from ks_path? msg?
                self.config_NSR(id)
            elif action == rwdts.QueryAction.READ:
                prepare_show_output()
                # Need loop here for each NSR?
                xact_info.respond_xpath(rwdts.XactRspCode.ACK,
                                        xpath=self._opdata_xpath,
                                        msg=self.show_output)

                # xact_info.respond_xpath(rwdts.XactRspCode.ACK)

        self._log.info("### Registering for cm-opdata xpath: %s",
                        self._opdata_xpath)

        handler=rift.tasklets.DTS.RegistrationHandler(on_prepare=on_prepare)
        try:
            #os.kill(os.getpid(), signal.SIGTRAP)
            yield from self._dts.register(xpath=self._opdata_xpath,
                                          handler=handler,
                                          flags=rwdts.Flag.PUBLISHER)
            self._log.info("Successfully registered (%s)", self._opdata_xpath)
        except Exception as e:
            self._log.error("Failed to register as (%s)", e)


    def process_nsd_vnf_configuration(self, nsr_obj, vnfr):

        def get_cfg_file_extension(method,  configuration_options):
            ext_dict = {
                "netconf" : "xml",
                "script" : {
                    "bash" : "sh",
                    "expect" : "exp",
                },
                "juju" : "yml"
            }

            if method == "netconf":
                return ext_dict[method]
            elif method == "script":
                return ext_dict[method][configuration_options['script_type']]
            elif method == "juju":
                return ext_dict[method]
            else:
                return "cfg"

        ## This is how the YAML file should look like, This routine will be called for each VNF, so keep appending the file.
        ## priority order is determined by the number, hence no need to generate the file in that order. A dictionary will be
        ## used that will take care of the order by number.
        '''
        1 : <== This is priority
          name : trafsink_vnfd
          member_vnf_index : 2
          configuration_delay : 120
          configuration_type : netconf
          configuration_options :
            username : admin
            password : admin
            port : 2022
            target : running
        2 :
          name : trafgen_vnfd
          member_vnf_index : 1
          configuration_delay : 0
          configuration_type : netconf
          configuration_options :
            username : admin
            password : admin
            port : 2022
            target : running
        '''

        # Save some parameters needed as short cuts in flat structure (Also generated)
        vnf_cfg = vnfr['vnf_cfg']
        # Prepare unique name for this VNF
        vnf_cfg['vnf_unique_name'] = get_vnf_unique_name(vnf_cfg['nsr_name'], vnfr['short_name'], vnfr['member_vnf_index_ref'])

        nsr_obj.this_nsr_dir = os.path.join(self._parent.cfg_dir, vnf_cfg['nsr_name'], self._nsr['name_ref'])
        if not os.path.exists(nsr_obj.this_nsr_dir):
            os.makedirs(nsr_obj.this_nsr_dir)
        nsr_obj.cfg_path_prefix = '{}/{}_{}'.format(nsr_obj.this_nsr_dir, vnfr['short_name'], vnfr['member_vnf_index_ref'])
        nsr_vnfr = '{}/{}_{}'.format(vnf_cfg['nsr_name'], vnfr['short_name'], vnfr['member_vnf_index_ref'])

        # Get vnf_configuration from vnfr
        vnf_config = vnfr['vnf_configuration']

        self._log.debug("vnf_configuration = %s", vnf_config)
        #print("### TBR ### vnf_configuration = ", vnf_config)

        # Create priority dictionary
        cfg_priority_order = 0
        if ('input_params' in vnf_config and
            'config_priority' in vnf_config['input_params']):
            cfg_priority_order = vnf_config['input_params']['config_priority']

        # All conditions must be met in order to process configuration
        if (cfg_priority_order != 0 and
            vnf_config['config_type'] is not None and
            vnf_config['config_type'] != 'none' and
            'config_template' in vnf_config):

            # Create all sub dictionaries first
            config_priority = {
                'name' : vnfr['short_name'],
                'member_vnf_index' : vnfr['member_vnf_index_ref'],
            }

            if 'config_delay' in vnf_config['input_params']:
                config_priority['configuration_delay'] = vnf_config['input_params']['config_delay']
                vnf_cfg['config_delay'] = config_priority['configuration_delay']

            configuration_options = {}
            method = vnf_config['config_type']
            config_priority['configuration_type'] = method
            vnf_cfg['config_method'] = method

            cfg_opt_list = ['port', 'target', 'script_type', 'ip_address', 'user', 'secret']
            for cfg_opt in cfg_opt_list:
                if cfg_opt in vnf_config[method]:
                    configuration_options[cfg_opt] = vnf_config[method][cfg_opt]
                    vnf_cfg[cfg_opt] = configuration_options[cfg_opt]

            cfg_opt_list = ['mgmt_ip_address', 'username', 'password']
            for cfg_opt in cfg_opt_list:
                if cfg_opt in vnf_config['config_access']:
                    configuration_options[cfg_opt] = vnf_config['config_access'][cfg_opt]
                    vnf_cfg[cfg_opt] = configuration_options[cfg_opt]

            # TBD - see if we can neatly include the config in "input_params" file, no need though
            #config_priority['config_template'] = vnf_config['config_template']
            # Create config file
            vnf_cfg['cfg_template'] = '{}_{}_template.cfg'.format(nsr_obj.cfg_path_prefix, config_priority['configuration_type'])
            vnf_cfg['cfg_file'] = '{}.{}'.format(nsr_obj.cfg_path_prefix, get_cfg_file_extension(method, configuration_options))
            vnf_cfg['xlate_script'] = os.path.join(self._parent.cfg_dir, 'xlate_cfg.py')
            vnf_cfg['juju_script'] = os.path.join(self._parent.cfg_dir, 'juju_if.py')

            try:
                # Now write this template into file
                with open(vnf_cfg['cfg_template'], "w") as cf:
                    cf.write(vnf_config['config_template'])
            except Exception as e:
                self._log.error("Processing NSD, failed to generate configuration template : %s (Error : %s)",
                                vnf_config['config_template'], str(e))
                raise

            self._log.debug("VNF endpoint so far: %s", vnf_cfg)

            # Populate filled up dictionary
            config_priority['configuration_options'] = configuration_options
            nsr_obj.nsr_cfg_input_params_dict[cfg_priority_order] = config_priority
            nsr_obj.num_vnfs_to_cfg += 1
            nsr_obj._vnfr_dict[vnf_cfg['vnf_unique_name']] = vnfr

            self._log.debug("input_params = %s", nsr_obj.nsr_cfg_input_params_dict)
        else:
            self._log.critical("NS/VNF %s is not to be configured by Configuration Manager!", nsr_vnfr)
            vnf_cfg['cm_state'] = conmanY.RecordState.READY_NO_CFG

    @asyncio.coroutine
    def config_NSR(self, id):
        nsr_dict = self._nsr_dict
        self._log.info("Initiate NSR fetch, id = %s", id)

        if id not in nsr_dict:
            nsr_obj = ConfigManagerNSR(self._log, self._loop, self._parent, id)
            nsr_dict[id] = nsr_obj
        else:
            self._log.warning("NSR(%s) is already initialized, skipping!", id)
            nsr_obj = nsr_dict[id]

        # Populate this object with netconfd API from RO

        # Get the ncclient handle (OR interface)
        orif = self._parent._event._orif

        if orif is None:
            self._log.error("OR interface not initialized")
        try:
            # Fetch NSR
            nsr = yield from orif.get_nsr(id)
            self._log.debug("nsr = (%s/%s)", type(nsr), nsr)
            self._nsr = nsr
            nsr_obj.state = conmanY.RecordState.RECEIVED

            try:
                # Parse NSR
                if nsr is not None:
                    nsr_obj.nsr_name = nsr['nsd_name_ref']
                    nsr_dir = os.path.join(self._parent.cfg_dir, nsr_obj.nsr_name)
                    self._log.info("Checking NS config directory: %s", nsr_dir)
                    if not os.path.isdir(nsr_dir):
                        os.makedirs(nsr_dir)
                        # self._log.critical("NS %s is not to be configured by Service Orchestrator!", nsr_obj.nsr_name)
                        # nsr_obj.state = conmanY.RecordState.READY_NO_CFG
                        # return

                    for vnfr_id in nsr['constituent_vnfr_ref']:
                        self._log.debug("Fetching VNFR (%s)", vnfr_id)
                        vnfr = yield from orif.get_vnfr(vnfr_id)
                        self._log.debug("vnfr = (%s/ %s)", type(vnfr), vnfr)
                        #print("### TBR ### vnfr = ", vnfr)
                        nsr_obj.add_vnfr(vnfr)
                        self.process_nsd_vnf_configuration(nsr_obj, vnfr)
            except Exception as e:
                self._log.error("Processing NSR (%s) Failed as (%s)", nsr_obj.nsr_name, e)
                nsr_obj.state = conmanY.RecordState.CFG_PROCESS_FAILED

            # Generate config_input_params.yaml (For debug reference)
            nsr_cfg_input_file = os.path.join(nsr_obj.this_nsr_dir, "configuration_input_params.yml")
            with open(nsr_cfg_input_file, "w") as yf:
                yf.write(yaml.dump(nsr_obj.nsr_cfg_input_params_dict, default_flow_style=False))

            self._log.debug("Starting to configure each VNF")

            ## Check if this NS has input parametrs
            self._log.info("Checking NS configuration order: %s", nsr_cfg_input_file)

            if os.path.exists(nsr_cfg_input_file):
                # Apply configuration is specified order
                try:
                    # Fetch number of VNFs
                    num_vnfs = nsr_obj.num_vnfs_to_cfg

                    # Go in loop to configure by specified order
                    self._log.info("Using Dynamic configuration input parametrs for NS: %s", nsr_obj.nsr_name)

                    # cfg_delay = nsr_obj.nsr_cfg_input_params_dict['configuration_delay']
                    # if cfg_delay:
                    #     self._log.info("Applying configuration delay for NS (%s) ; %d seconds",
                    #                    nsr_obj.nsr_name, cfg_delay)
                    #     yield from asyncio.sleep(cfg_delay, loop=self._loop)

                    for i in range(1,num_vnfs+1):
                        if i not in nsr_obj.nsr_cfg_input_params_dict:
                            self._log.warning("NS (%s) - Ordered configuration is missing order-number: %d", nsr_obj.nsr_name, i)
                        else:
                            vnf_input_params_dict = nsr_obj.nsr_cfg_input_params_dict[i]

                            # Make up vnf_unique_name with vnfd name and member index
                            #vnfr_name = "{}.{}".format(nsr_obj.nsr_name, vnf_input_params_dict['name'])
                            vnf_unique_name = get_vnf_unique_name(
                                    nsr_obj.nsr_name,
                                    vnf_input_params_dict['name'],
                                    str(vnf_input_params_dict['member_vnf_index']),
                                    )
                            self._log.info("NS (%s) : VNF (%s) - Processing configuration input params",
                                           nsr_obj.nsr_name, vnf_unique_name)

                            # Find vnfr for this vnf_unique_name
                            if vnf_unique_name not in nsr_obj._vnfr_dict:
                                self._log.error("NS (%s) - Can not find VNF to be configured: %s", nsr_obj.nsr_name, vnf_unique_name)
                            else:
                                # Save this unique VNF's config input parameters
                                nsr_obj.vnf_input_params_dict[vnf_unique_name] = vnf_input_params_dict
                                nsr_obj.ConfigVNF(nsr_obj._vnfr_dict[vnf_unique_name], nsr_obj)

                except Exception as e:
                    self._log.error("Failed processing input parameters for NS (%s) : %s", nsr_obj.nsr_name, str(e))
                    pass
            else:
                self._log.error("No configuration input parameters for NSR (%s)", nsr_obj.nsr_name)

        except Exception as e:
            self._log.error("config_NSR Failed as (%s)", e)
            nsr_obj.state = conmanY.RecordState.CFG_PROCESS_FAILED

    @asyncio.coroutine
    def terminate_NSR(self, id):
        nsr_dict = self._nsr_dict
        if id not in nsr_dict:
            self._log.error("NSR(%s) does not exist!", id)
            return
        else:
            nsr_obj = nsr_dict.pop(id, None)
            # Also remove any scheduled configuration event
            for vnf_cfg in self._parent.pending_cfg:
                if vnf_cfg['nsr_id'] == id:
                    self._parent.pending_cfg.remove(vnf_cfg)
                    self._log.critical("Removed scheduled configuration for NSR(%s)/VNF(%s/%d)", vnf_cfg['nsr_name'], vnf_cfg['vnfr_name'], vnf_cfg['member_vnf_index'])
            self._log.critical("NSR(%s/%s) is deleted", nsr_obj.nsr_name, id)
                

class ConfigManagerNSR(object):
    def __init__(self, log, loop, parent, id):
        self._log = log
        self._loop = loop
        self._rwcal = None
        self._vnfr_dict = {}
        self._cp_dict = {}
        self._nsr_id = id
        self._parent = parent
        self.state = conmanY.RecordState.INIT
        self._log.info("Instantiated NSR entry for id=%s", id)
        self.nsr_cfg_input_params_dict = {}
        self.vnf_input_params_dict = {}
        self.num_vnfs_to_cfg = 0
        self._vnfr_list = []
        self.nsr_name = 'Not Set'

    def xlate_conf(self, vnfr, vnf_cfg):

        # If configuration type is not already set, try to read from input params
        if vnf_cfg['interface_type'] is None:
            # Prepare unique name for this VNF
            vnf_unique_name = get_vnf_unique_name(
                    vnf_cfg['nsr_name'],
                    vnfr['short_name'],
                    vnfr['member_vnf_index_ref'],
                    )

            # Find this particular (unique) VNF's config input params
            if (vnf_unique_name in self.vnf_input_params_dict):
                vnf_cfg_input_params_dict = self.vnf_input_params_dict[vnf_unique_name]
                vnf_cfg['interface_type'] = vnf_cfg_input_params_dict['configuration_type']
                if 'configuration_options' in vnf_cfg_input_params_dict:
                    cfg_opts = vnf_cfg_input_params_dict['configuration_options']
                    for key, value in cfg_opts.items():
                        vnf_cfg[key] = value

        cfg_path_prefix = '{}/{}/{}_{}'.format(
                self._parent.cfg_dir,
                vnf_cfg['nsr_name'],
                vnfr['short_name'],
                vnfr['member_vnf_index_ref'],
                )

        # ''' This should be in VNFR or somehow provided to SO - TBD '''
        vnf_cfg['cfg_template'] = '{}_{}_template.cfg'.format(cfg_path_prefix, vnf_cfg['interface_type'])
        vnf_cfg['cfg_file'] = '{}.cfg'.format(cfg_path_prefix)
        vnf_cfg['xlate_script'] = self._parent.cfg_dir + '/xlate_cfg.py'

        self._log.debug("VNF endpoint so far: %s", vnf_cfg)

        self._log.info("Checking cfg_template %s", vnf_cfg['cfg_template'])
        if os.path.exists(vnf_cfg['cfg_template']):
            return True
        return False

    def ConfigVNF(self, vnfr, nsr_obj):
        vnf_cfg = vnfr['vnf_cfg']

        if vnf_cfg['cm_state'] != conmanY.RecordState.RECEIVED:
            self._log.warning("NS/VNF (%s/%s) may be reconfigured!", nsr_obj.nsr_name, vnfr['name'])

        #UPdate VNF state
        vnf_cfg['cm_state'] = conmanY.RecordState.CFG_PROCESS

        # Now translate the configuration for iP addresses
        try:
            #print("### TBR ### vnf_cfg = ", vnf_cfg)
            # Add cp_dict members (TAGS) for this VNF
            self._cp_dict['rw_mgmt_ip'] = vnf_cfg['mgmt_ip_address']
            self._cp_dict['rw_username'] = vnf_cfg['username']
            self._cp_dict['rw_password'] = vnf_cfg['password']

            script_cmd = 'python3 {} -i {} -o {} -x "{}"'.format(vnf_cfg['xlate_script'], vnf_cfg['cfg_template'], vnf_cfg['cfg_file'], repr(self._cp_dict))
            self._log.debug("xlate script command (%s)", script_cmd)
            #xlate_msg = subprocess.check_output(script_cmd).decode('utf-8')
            xlate_msg = subprocess.check_output(script_cmd, shell=True).decode('utf-8')
            self._log.info("xlate script output (%s)", xlate_msg)
        except Exception as e:
            vnf_cfg['cm_state'] = conmanY.RecordState.CFG_PROCESS_FAILED
            self._log.error("Failed to execute translation script with (%s)", str(e))
            return

        self._log.info("Applying config to VNF(%s) = %s!", vnfr['name'], vnf_cfg)
        try:
            self._parent.pending_cfg.append(vnf_cfg)
            self._log.debug("Scheduled configuration!")
            vnf_cfg['cm_state'] = conmanY.RecordState.CFG_SCHED
        except Exception as e:
            self._log.error("apply_vnf_config Failed as (%s)", e)
            vnf_cfg['cm_state'] = conmanY.RecordState.CFG_PROCESS_FAILED
            raise

    def add(self, nsr):
        self._log.info("Adding NS Record for id=%s", id)
        self._nsr = nsr

    def add_vnfr(self, vnfr):

        if vnfr['id'] not in self._vnfr_dict:
            self._log.info("NSR(%s) : Adding VNF Record for name=%s, id=%s", self._nsr_id, vnfr['name'], vnfr['id'])
            # Add this vnfr to the list for show, or single traversal
            self._vnfr_list.append(vnfr)
        else:
            self._log.warning("NSR(%s) : VNF Record for name=%s, id=%s already exists, overwriting", self._nsr_id, vnfr['name'], vnfr['id'])

        # Make vnfr available by id as well as by name
        unique_name = get_vnf_unique_name(self.nsr_name, vnfr['short_name'], vnfr['member_vnf_index_ref'])
        self._vnfr_dict[unique_name] = vnfr
        self._vnfr_dict[vnfr['id']] = vnfr

        # Create vnf_cfg dictionary with default values
        vnf_cfg = {
            'cm_state' : conmanY.RecordState.RECEIVED,
            'vnfr' : vnfr,
            'nsr_name' : self.nsr_name,
            'nsr_id' : self._nsr_id,
            'vnfr_name' : vnfr['short_name'],
            'member_vnf_index' : vnfr['member_vnf_index_ref'],
            'port' : 0,
            'username' : 'admin',
            'password' : 'admin',
            'config_method' : 'None',
            'protocol' : 'None',
            'mgmt_ip_address' : '0.0.0.0',
            'cfg_file' : 'None',
            'script_type' : 'bash',
            }

        vnfr['vnf_cfg'] = vnf_cfg


        '''
        Build the connection-points list for this VNF (self._cp_dict)
        '''
        # Populate global CP list self._cp_dict from VNFR
        cp_list = vnfr['connection_point']

        self._cp_dict[vnfr['member_vnf_index_ref']] = {}
        for cp_item_dict in cp_list:
            # Populate global dictionary
            self._cp_dict[cp_item_dict['name']] = cp_item_dict['ip_address']

            # Populate unique member specific dictionary
            self._cp_dict[vnfr['member_vnf_index_ref']][cp_item_dict['name']] = cp_item_dict['ip_address']

        return


