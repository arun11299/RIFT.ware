# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import asyncio
import concurrent.futures
import re
import sys
import tempfile
import yaml

from gi.repository import (
    RwDts as rwdts,
)

from . import rwnsmconfigplugin
from . import juju_intf


# Charm service name accepts only a to z and -.
def get_vnf_unique_name(nsr_name, vnfr_short_name, member_vnf_index):
    name = "{}-{}-{}".format(nsr_name, vnfr_short_name, member_vnf_index)
    new_name = ''
    for c in name:
        if c.isdigit():
            c = chr(97 + int(c))
        elif not c.isalpha():
            c = "-"
        new_name += c
    return new_name.lower()

class JujuExecuteHelper(object):
    ''' Run Juju API calls that dwe do not need to wait for response '''
    def __init__(self, log, loop):
        self._log = log
        self._loop = loop
        self._executor = concurrent.futures.ThreadPoolExecutor(max_workers=1)

    @property
    def loop(self):
        return self._loop

    @property
    def log(self):
        return self._log

    @property
    def executor(self):
        return self._executor

    @asyncio.coroutine
    def deploy_service(self, api, charm, service):
        self.log.debug("Deploying service using %s as %s" % (charm, service))
        try:
            rc = yield from self.loop.run_in_executor(
                self.executor,
                api._deploy_service,
                charm, service
            )
            self.log.info("Deploy service %s returned %s" % (service, rc))
        except Exception as e:
            self.log.error("Error deploying service {}, e={}".
                           format(service, e))
        self.log.debug("Deployed service using %s as %s " % (charm, service))

    @asyncio.coroutine
    def destroy_service(self, api, service):
        self.log.debug("Destroying service %s" % (service))
        yield from self.loop.run_in_executor(
            self.executor,
            api._destroy_service,
            service
        )
        self.log.debug("Destroyed service %s" % (service))


class JujuNsmConfigPlugin(rwnsmconfigplugin.NsmConfigPluginBase):
    """
        Juju implementation of the NsmConfPluginBase
    """
    def __init__(self, dts, log, loop, publisher, account):
        rwnsmconfigplugin.NsmConfigPluginBase.__init__(self, dts, log, loop, publisher, account)
        self._name = account.name
        self._ip_address = account.juju.ip_address
        self._port = account.juju.port
        self._user = account.juju.user
        self._secret = account.juju.secret
        self._api = None
        self._juju_vnfs = {}
        self._helper = JujuExecuteHelper(log, loop)
        self._tasks = {}

    # TBD: Do a better, similar to config manager
    def xlate(self, tag, tags):
        # TBD
        if tag is None:
            return tag
        val = tag
        if re.search('<.*>', tag):
            self._log.debug("Juju config agent: Xlate value %s" % tag)
            try:
                if tag == '<rw_mgmt_ip>':
                    val = tags['rw_mgmt_ip']
            except KeyError as e:
                self._log.info("Juju config agent: Did not get a value for tag %s, e=%s" % (tag, e))
        return val

    @asyncio.coroutine
    def notify_create_nsr(self, nsr, nsd):
        """
        Notification of create Network service record
        """
        pass


    @asyncio.coroutine
    def notify_create_vls(self, nsr, vld, vlr):
        """
        Notification of create VL record
        """
        pass

    @asyncio.coroutine
    def notify_create_vnfr(self, nsr, vnfr):
        """
        Notification of create Network VNF record
        Returns True if configured using config_agent
        """
        # Deploy the charm if specified for the vnf
        self._log.debug("Juju config agent: create vnfr nsr=%s  vnfr=%s" %(nsr, vnfr.id))
        self._log.debug("Juju config agent: Const = %s" %(vnfr._const_vnfd))
        try:
            vnf_config = vnfr._const_vnfd.vnf_configuration
            self._log.debug("Juju config agent: vnf_configuration = %s", vnf_config)
            if vnf_config.config_type != 'juju':
                return False
            charm = vnf_config.juju.charm
            self._log.debug("Juju config agent: charm = %s", charm)
        except:
            self._log.debug("Juju config agent: vnf_configuration error %s" % (sys.exc_info()[0]))
            return False

        # Add a instance api for use with exectuon status
        if self._api is None:
            # Create a api instance to get the action status
            self._api = yield from self._loop.run_in_executor(
                None,
                juju_intf.JujuApi,
                self._log, self._ip_address,
                self._port, self._user, self._secret,
                self.loop
            )

        # Prepare unique name for this VNF
        vnf_unique_name = get_vnf_unique_name(vnfr._nsr_name, vnfr.vnfd.name, vnfr.member_vnf_index)
        if vnf_unique_name in self._juju_vnfs:
            self._log.warn("Juju config agent: Service %s already deployed" % (vnf_unique_name))
            api = self._juju_vnfs['api']
        else:
            # Create an juju api instance for this VNFR
            api = yield from self._loop.run_in_executor(
                    None,
                    juju_intf.JujuApi,
                    self._log, self._ip_address,
                    self._port, self._user, self._secret,
                    self.loop
                )
        self._juju_vnfs.update({vnfr.id: {'name': vnf_unique_name, 'charm': charm,
                                          'nsr_id': nsr, 'member_vnf_index': vnfr.member_vnf_index,
                                          'xpath': vnfr.xpath, 'tags': {},
                                          'active': False, 'config': vnf_config,
                                          'api' : api}})
        self._log.debug("Juju config agent: Charm %s for vnf %s deployed as %s" %
                        (charm, vnfr.name, vnf_unique_name))

        try:
            if vnf_unique_name not in self._tasks:
                self._tasks[vnf_unique_name] = {}
            self._tasks[vnf_unique_name]['deploy'] = self.loop.create_task(
                self._helper.deploy_service(api, charm, vnf_unique_name)
            )
        except Exception as e:
            self._log.critical("Juju config agent: Unable to deploy service %s for charm %s: {}" %
                               (vnf_unique_name, charm, e))

        self._log.debug("Juju config agent: Deployed service %s" % vnf_unique_name)
        return True

    @asyncio.coroutine
    def notify_instantiate_ns(self, nsr):
        """
        Notification of NSR instantiationwith the passed nsr id
        """
        pass

    @asyncio.coroutine
    def notify_instantiate_vnf(self, nsr, vnfr, xact):
        """
        Notification of Instantiate NSR with the passed nsr id
        """
        pass

    @asyncio.coroutine
    def notify_instantiate_vl(self, nsr, vlr, xact):
        """
        Notification of Instantiate NSR with the passed nsr id
        """
        pass

    @asyncio.coroutine
    def notify_nsr_active(self, nsr, vnfrs):
        """ Notify instantiate of the virtual link"""
        for vnf in vnfrs.values():
            self._log.debug("Juju config agent: ns active VNF %s" % vnf.name)
            try:
                if vnf.id in self._juju_vnfs.keys():
                    #self._log.debug("Juju config agent: Fetching VNF: %s in NS %s", vnf.name, nsr)
                    # vnfr = yield from self.fetch_vnfr(vnf.xpath)

                    # Check if the deploy is done
                    if self.check_task_status(self._juju_vnfs[vnf.id]['name'], 'deploy'):
                        # apply initial config for the vnfr
                        yield from self.apply_initial_config(vnf.id, vnf)
                    else:
                        self._log.info("Juju config agent: service not yet deployed for %s" % vnf.id)
            except Exception as e:
                self._log.error("Juju config agent: ns active VNF {}, e {}".format(vnf.name, e))

    @asyncio.coroutine
    def notify_terminate_ns(self, nsr):
        """
        Notification of Terminate the network service
        """
        pass

    @asyncio.coroutine
    def notify_terminate_vnf(self, nsr, vnfr, xact):
        """
        Notification of Terminate the network service
        """
        self._log.debug("Juju config agent: Terminate VNFr {}, current vnfrs={}".
                        format(vnfr.id, self._juju_vnfs))
        try:
            vnf = self._juju_vnfs[vnfr.id]
            service = vnf['name']
            self._log.debug ("Juju config agent: Terminating VNFr %s, %s" %
                             (vnfr.id, service))
            self._tasks[service]['destroy'] = self.loop.create_task(
                self._helper.destroy_service(vnf['api'], service)
            )
            del vnf
            self._log.debug ("Juju config agent: current vnfrs={}".
                             format(self._juju_vnfs))
            if service in self._tasks:
                tasks = []
                for action in self._tasks[service].keys():
                    #if self.check_task_status(service, action):
                    tasks.append(action)
                del tasks
        except KeyError as e:
            self._log.debug ("Juju config agent: Error termiating charm service for VNFr {}, e={}".
                             format(vnfr.id, e))

    @asyncio.coroutine
    def notify_terminate_vl(self, nsr, vlr, xact):
        """
        Notification of Terminate the virtual link
        """
        pass

    def check_task_status(self, service, action):
        #self.log.debug("Juju config agent: check task status for %s, %s" % (service, action))
        try:
            task = self._tasks[service][action]
            if task.done():
                self.log.debug("Juju config agent: Task for %s, %s done" % (service, action))
                e = task.exception()
                if e:
                    self.log.error("Juju config agent: Error in task for {} and {} : {}".
                                   format(service, action, e))
                    return False
                r= task.result()
                if r:
                    self.log.debug("Juju config agent: Task for {} and {}, returned {}".
                                   format(service, action,r))
                return True
            else:
                self.log.debug("Juju config agent: task {}, {} not done".
                               format(service, action))
        except KeyError as e:
            self.log.error("Juju config agent: KeyError for task for {} and {}: {}".
                           format(service, action, e))
        except Exception as e:
            self.log.error("Juju config agent: Error for task for {} and {}: {}".
                           format(service, action, e))
        return False

    @asyncio.coroutine
    def vnf_config_primitive(self, nsr_id, vnfr_id, primitive, output):
        self._log.debug("Juju config agent: VNF config primititve {} for nsr {}, vnfr_id {}".
                        format(primitive, nsr_id, vnfr_id))
        output.execution_status = "failed"
        output.execution_id = ''
        try:
            vnfr = self._juju_vnfs[vnfr_id]
        except KeyError:
            self._log.error("Juju config agent: Did not find VNFR %s in juju plugin" % vnfr_id)
            return

        try:
            service = vnfr['name']
            vnf_config = vnfr['config']
            api = vnfr['api']
            self._log.debug("VNF config %s" % vnf_config)
            configs = vnf_config.config_primitive
            for config in configs:
                if config.name == primitive.name:
                    self._log.debug("Juju config agent: Found the config primitive %s" % config.name)
                    params = {}
                    for parameter in primitive.parameter:
                        if parameter.value:
                            val = self.xlate(parameter.value, vnfr['tags'])
                            # TBD do validation of the parameters
                            data_type = 'string'
                            found = False
                            for ca_param in config.parameter:
                                if ca_param.name == parameter.name:
                                    data_type = ca_param.data_type
                                    found = True
                                    break
                                if data_type == 'integer':
                                    val = int(parameter.value)
                            if not found:
                                self._log.warn("Juju config agent: Did not find parameter {} for {}".
                                               format(parameter, config.name))
                            params.update({parameter.name: val})
                    if config.name == 'config':
                        if len(params):
                            self._log.debug("Juju config agent: applying config with params {} for service {}".
                                            format(params, service))
                            rc = yield from self._loop.run_in_executor(
                                None,
                                api._apply_config,
                                service,
                                params
                            )
                            if rc:
                                output.execution_status = "completed"
                                self._log.debug("Juju config agent: applied config {} on {}".
                                                format(params, service))
                                # Removing this as clearwater has fixed its config hook
                                # Sleep for sometime for the config to take effect
                                # self._log.debug("Juju config agent: Wait sometime for config to take effect")
                                # yield from self._loop.run_in_executor(
                                #     None,
                                #     time.sleep,
                                #     30
                                # )
                                # self._log.debug("Juju config agent: Wait over for config to take effect")
                            else:
                                output.execution_status = 'failed'
                                self._log.error("Juju config agent: Error applying config {} on service {}".
                                                format(params, service))
                        else:
                            self._log.warn("Juju config agent: Did not find valid paramaters for config : {}".
                                           format(primitive.parameter))
                    else:
                        self._log.debug("Juju config agent: Execute action {} on service {} with params {}".
                                        format(config.name, service, params))
                        tags = yield from self._loop.run_in_executor(
                            None,
                            api._execute_actions,
                            service, config.name, params
                        )
                        if len(tags):
                            output.execution_id = tags[0]
                            output.execution_status = api.get_action_status(tags[0])
                            self._log.debug("Juju config agent: excute action {} on service {} returned {}".
                                            format(config.name, service, output.execution_status))
                        else:
                            self._log.error("Juju config agent: error executing action {} for {} with {}".
                                            format(config.name, service, params))
                            output.execution_id = ''
                            output.execution_status = 'failed'
                    break
        except KeyError as e:
            self._log.info("VNF %s does not have config primititves, e=%s" % (vnfr_id, e))

    @asyncio.coroutine
    def apply_config(self, rpc_ip, nsr, vnfrs):
        """Hook: Runs the user defined script. Feeds all the necessary data
        for the script thro' yaml file.

        Args:
            rpc_ip (YangInput_Nsr_ExecNsConfigPrimitive): The input data.
            nsr (NetworkServiceRecord): Description
            vnfrs (dict): VNFR ID => VirtualNetworkFunctionRecord
        """
        def get_meta(vnfrs):
            unit_names, initial_params, vnfr_index_map = {}, {}, {}

            for vnfr_id, juju_vnf in self._juju_vnfs.items():
                # Only add vnfr info for vnfs in this particular nsr
                if vnfr_id not in nsr.vnfrs:
                    continue
                # Vnfr -> index ref
                vnfr_index_map[vnfr_id] = juju_vnf['member_vnf_index']

                # Unit name
                unit_names[vnfr_id] = juju_vnf['name']

                # Flatten the data for simplicity
                param_data = {}
                for primitive in juju_vnf['config'].initial_config_primitive:
                    for parameter in primitive.parameter:
                        value = self.xlate(parameter.value, juju_vnf['tags'])
                        param_data[parameter.name] = value

                initial_params[vnfr_id] = param_data


            return unit_names, initial_params, vnfr_index_map

        for vnfr_id, vnf in self._juju_vnfs.items():
            print (vnf['config'].as_dict())

        unit_names, init_data, vnfr_index_map = get_meta(vnfrs)

        # The data consists of 4 sections
        # 1. Account data
        # 2. The input passed.
        # 3. Juju unit names (keyed by vnfr ID).
        # 4. Initial config data (keyed by vnfr ID).
        data = dict()
        data['config_agent'] = dict(
                name=self._name,
                host=self._ip_address,
                port=self._port,
                user=self._user,
                secret=self._secret
                )
        data["rpc_ip"] = rpc_ip.as_dict()
        data["unit_names"] = unit_names
        data["init_config"] = init_data
        data["vnfr_index_map"] = vnfr_index_map

        tmp_file = None
        with tempfile.NamedTemporaryFile(delete=False) as tmp_file:
            tmp_file.write(yaml.dump(data, default_flow_style=True)
                    .encode("UTF-8"))

        self._log.debug("Juju config agent: Creating a temp file: {} with input data".format(
                tmp_file.name))

        cmd = "{} {}".format(rpc_ip.user_defined_script, tmp_file.name)
        self._log.debug("Juju config agent: Running the CMD: {}".format(cmd))

        coro = asyncio.create_subprocess_shell(cmd, loop=self._loop)
        process = yield from coro
        task = self._loop.create_task(process.wait())

        return task

    @asyncio.coroutine
    def fetch_vnfr(self, vnfr_path):
        """ Fetch VNFR record """
        vnfr = None
        self._log.debug("Juju config agent: Fetching VNFR with key %s", vnfr_path)
        res_iter = yield from self._dts.query_read(vnfr_path, rwdts.Flag.MERGE)

        for ent in res_iter:
            res = yield from ent
            vnfr = res.result

        return vnfr

    @asyncio.coroutine
    def apply_initial_config(self, vnf_id, vnf):
        try:
            tags = []
            vnfr = self._juju_vnfs[vnf_id]
            vnf_cat = yield from self.fetch_vnfr(vnf.xpath)
            if vnf_cat and vnf_cat.mgmt_interface.ip_address:
                vnfr['tags'].update({'rw_mgmt_ip': vnf_cat.mgmt_interface.ip_address})
            config = {}
            try:
                for primitive in vnfr['config'].initial_config_primitive:
                    self._log.debug("Initial config %s" % (primitive))
                    # Currently, expecting only config, not actions, for initial config
                    if primitive.name == 'config':
                        for param in primitive.parameter:
                            if vnfr['tags']:
                                val = self.xlate(param.value, vnfr['tags'])
                                config.update({param.name: val})
            except KeyError as e:
                self._log.exception("Juju config agent: Initial config error: config=%s" % config)
                config = None
                return
            self._log.debug("Juju config agent: Applying initial config for %s as %s" %
                            (vnfr['name'], config))
            yield from self._loop.run_in_executor(
                None,
                vnfr['api']._apply_config,
                vnfr['name'],
                config
            )

            # Apply any actions specified as part of initial config
            for primitive in vnfr['config'].initial_config_primitive:
                self._log.debug("Juju config agent: Initial config %s" % (primitive))
                if primitive.name != 'config':
                    action = primitive.name
                    params = {}
                    for param in primitive.parameter:
                        val = self.xlate(param.value, self._juju_vnfs[vnf_id]['tags'])
                        params.update({param.name: val})

                    self._log.debug("Juju config agent: Action %s with params %s" % (action, params))
                    tag = yield from self._loop.run_in_executor(
                        None,
                        vnfr['api']._execute_actions,
                        vnfr['name'],
                        action, params
                    )
                    tags.append(tag)

        except KeyError as e:
            self._log.info("Juju config agent: VNFR %s not managed by Juju, e=%s" % (vnf_id, e))
        except Exception as e:
            self._log.exception("Juju config agent: Exception juju apply_initial_config for VNFR %s",
                                vnf_id)
        return tags

    def is_vnfr_managed(self, vnfr_id):
        try:
            if self._juju_vnfs[vnfr_id]:
                return True
        except:
            pass
        return False

    def is_service_active(self, service):
        """ Is the juju service active  """
        resp = False
        try:
            for vnf in self._juju_vnfrs:
                if vnf['name'] == service:
                    # Check if deploy is over
                    if self.check_task_status(service, 'deploy'):
                        resp = yield from self._loop.run_in_executor(
                            None,
                            vnf['api'].is_service_active,
                            service
                        )
                    self._log.debug("Juju config agent: Is the service %s active? %s", service, resp)
                    return resp
        except KeyError:
            self._log.error("Juju config agent: Check active unknown service ", service)
        except Exception as e:
            self._log.error("Juju config agent: Caught exception when checking for service is active: ", e)
        return resp

    @asyncio.coroutine
    def is_configured(self, vnfr_id):
        try:
            if self._juju_vnfs[vnfr_id]['active']:
                return True

            service = self._juju_vnfs[vnfr_id]['name']
            resp = self.is_service_active(service)
            self._juju_vnfs[vnfr_id]['active'] = resp
            self._log.debug("Juju config agent: Service state for {} is {}".
                            format(service, resp))
            return resp
        except KeyError as e:
            self._log.error("Juju config agent: VNFR id %s not found in config agent, e=%s" %
                            (vnfr_id, e))
            return True

    @asyncio.coroutine
    def get_status(self, vnfr_id):
        resp = 'unknown'
        try:
            vnfr = self._juju_vnfs[vnfr_id]
            if vnfr['active']:
                return 'configured'

            service = vnfr['name']
            # Check if deploy is over
            if self.check_task_status(service, 'deploy'):
                resp = yield from self._loop.run_in_executor(
                    None,
                    vnfr['api'].get_service_status,
                    service
                )
            self._log.debug("Juju config agent: Service status for {} is {}".
                            format(service, resp))
            status =  'configuring'
            if resp is None:
                status = 'failed'
            elif resp in ['active', 'blocked']:
                status = 'configured'
            elif resp in ['error']:
                status = 'failed'
            return status
        except KeyError as e:
            self._log.error("Juju config agent: VNFR id %s not found in config agent, e=%s" %
                            (vnfr_id, e))
            return 'configured'

    def get_action_status(self, execution_id):
        ''' Get the action status for an execution ID
            *** Make sure this is NOT a asyncio coroutine function ***
        '''
        try:
            if self._api:
                return self._api.get_action_status(execution_id)
            else:
                self._log.error("Juju config agent: common API not instantiated")
        except Exception:
            self._log.exception("Juju config agent: Error fetching execution status for %s",
                                execution_id)
            return None
