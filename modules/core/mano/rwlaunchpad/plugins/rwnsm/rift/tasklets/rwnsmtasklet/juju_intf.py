# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

# Part of the code taken from
# https://github.com/chuckbutler/juju_action_api_class/blob/master/juju_actions.py

import asyncio
import jujuclient
import sys
import ssl
import os
import time


class Action(object):
    def __init__(self, data):
        # I am undecided if we need this
        # model_id = ""
        self.uuid = data['action']['tag']
        self.data = data  # straight from juju api
        self.juju_status = data['status']

    @classmethod
    def from_data(cls, data):
        o = cls(data=data)
        return o


def get_service_units(status):
    results = {}
    services = status.get('Services', {})
    for svc_name, svc_data in services.items():
        units = svc_data['Units'] or {}
        sub_to = svc_data['SubordinateTo']
        if not units and sub_to:
            for sub in sub_to:
                for unit_name, unit_data in \
                        (services[sub].get('Units') or {}).items():
                    for sub_name, sub_data in \
                            (unit_data['Subordinates'] or {}).items():
                        if sub_name.startswith(svc_name):
                            units[sub_name] = sub_data
        results[svc_name] = units
    return results


class ApiEnvironment(jujuclient.Environment):
    def actions_available(self, service=None):
        args = {
            "Type": 'Action',
            "Request": 'ServicesCharmActions',
            "Params": {
                "Entities": []
            }
        }

        services = self.status().get('Services', {})
        service_names = [service] if service else services
        for name in service_names:
            args['Params']['Entities'].append(
                {
                    "Tag": 'service-' + name
                }
            )

        return self._rpc(args)

    def actions_list_all(self, service=None):
        args = {
            "Type": 'Action',
            "Request": 'ListAll',
            "Params": {
                "Entities": []
            }
        }

        service_units = get_service_units(self.status())
        service_names = [service] if service else service_units.keys()
        units = []

        for name in service_names:
            units += service_units[name].keys()

        for unit in set(units):
            args['Params']['Entities'].append(
                {
                    "Tag": "unit-%s" % unit.replace('/', '-'),
                }
            )

        return self._rpc(args)

    def actions_enqueue(self, action, receivers, params=None):
        args = {
            "Type": "Action",
            "Request": "Enqueue",
            "Params": {
                "Actions": []
            }
        }

        for receiver in receivers:
            args['Params']['Actions'].append({
                "Receiver": receiver,
                "Name": action,
                "Parameters": params or {},
            })

        return self._rpc(args)

    def actions_cancel(self, uuid):
        return self._rpc({
            'Type': 'Action',
            'Request': 'Cancel',
            "Params": {
                "Entities": [{'Tag': 'action-' + uuid}]
            }
        })


def _parse_action_specs(api_results):
    results = {}

    r = api_results['results']
    for service in r:
        servicetag = service['servicetag']
        service_name = servicetag[8:]  # remove 'service-' prefix
        specs = {}
        if service['actions']['ActionSpecs']:
            for spec_name, spec_def in \
                    service['actions']['ActionSpecs'].items():
                specs[spec_name] = ActionSpec(spec_name, spec_def)
        results[service_name] = specs
    return results


def _parse_action_properties(action_properties_dict):
    results = {}

    d = action_properties_dict
    for prop_name, prop_def in d.items():
        results[prop_name] = ActionProperty(prop_name, prop_def)
    return results


class Dict(dict):
    def __getattr__(self, name):
        return self[name]


class ActionSpec(Dict):
    def __init__(self, name, data_dict):
        params = data_dict['Params']
        super(ActionSpec, self).__init__(
            name=name,
            title=params['title'],
            description=params['description'],
            properties=_parse_action_properties(params['properties'])
        )


class ActionProperty(Dict):
    types = {
        'string': str,
        'integer': int,
        'boolean': bool,
        'number': float,
    }
    type_checks = {
        str: 'string',
        int: 'integer',
        bool: 'boolean',
        float: 'number',
    }

    def __init__(self, name, data_dict):
        super(ActionProperty, self).__init__(
            name=name,
            description=data_dict.get('description', ''),
            default=data_dict.get('default', ''),
            type=data_dict.get(
                'type', self._infer_type(data_dict.get('default'))),
        )

    def _infer_type(self, default):
        if default is None:
            return 'string'
        for _type in self.type_checks:
            if isinstance(default, _type):
                return self.type_checks[_type]
        return 'string'

    def to_python(self, value):
        f = self.types.get(self.type)
        return f(value) if f else value


class JujuApi(object):
    def __init__ (self, log, server, port, user, secret, loop):
        self.log = log
        self.server = server
        self.user = user
        self.port = port
        self.secret = secret
        self.loop = loop
        endpoint = 'wss://%s:%d' % (server.split()[0], int(port))
        log.info("Juju: API endpoint %s" % endpoint)
        self.endpoint = endpoint
        self.env = ApiEnvironment(endpoint)
        self.env.login(secret, user=user)
        self.deploy_timeout = 600
        self.lock = asyncio.Semaphore(loop=loop)
        # Check python version and setup up SSL
        if sys.version_info >= (3,4):
            # This is needed for python 3.4 above as by default certificate
            # validation is enabled
            ssl._create_default_https_context = ssl._create_unverified_context

    def reconnect(self):
        self.log.info("Juju: try reconnect to endpoint ", self.endpoint)
        try:
            self.env.close()
            del self.env
        except Exception as e:
            self.log.debug("Juju: env close threw e {}".
                           format(e))

        try:
            self.env = ApiEnvironment(self.endpoint)
            self.env.login(self.secret, user=self.user)
            self.log.info("Juju: reconnected to endpoint ", self.endpoint)
        except Exception as e:
            self.log.error("Juju: exception in close e={}".format(e))


    def get_status(self):
        try:
            status = self.env.status()
            return status
        except Exception as e:
            self.log.error("Juju: exception while getting status: {}".format(e))
            self.reconnect()
        return None

    def get_annotations(self, services):
        '''
        Return dict of (servicename: annotations) for each servicename
        in `services`.
        '''
        if not services:
            return None

        d = {}
        for s in services:
            d[s] = self.env.get_annotation(s, 'service')['Annotations']
        return d

    def get_actions(self, service=None):
        return self.env.actions_list_all(service)

    def get_action_status(self, action_tag):
        '''
        responds with the action status, which is one of three values:

         - completed
         - pending
         - failed

         @param action_tag - the action UUID return from the enqueue method
         eg: action-3428e20d-fcd7-4911-803b-9b857a2e5ec9
        '''
        try:
            self.lock.acquire()
            receiver = self.get_actions()
            for receiver in receiver['actions']:
                if 'actions' in receiver.keys():
                    for action_record in receiver['actions']:
                        if 'action' in action_record.keys():
                            if action_record['action']['tag'] == action_tag:
                                self.lock.release()
                                return action_record['status']
        except Exception as e:
            self.log.error("Juju: exception in get action status {}".format(e))

        self.lock.release()

    def cancel_action(self, uuid):
        return self.env.actions_cancel(uuid)

    def get_service_units(self):
        return get_service_units(self.env.status())

    def get_action_specs(self):
        results = self.env.actions_available()
        return _parse_action_specs(results)

    def enqueue_action(self, action, receivers, params):
        result = self.env.actions_enqueue(action, receivers, params)
        try:
            return Action.from_data(result['results'][0])
        except Exception as e:
            self.log.error("Juju: Exception enqueing action {} on units {} with params {}: {}".
                           format(action, receivers, params, e))
            return None

    @asyncio.coroutine
    def is_deployed(self, service):
        return self._is_deployed(service)

    def _is_deployed(self, service, status=None):
        status = self.get_service_status(service, status=status)
        if status:
            if status not in ['terminated']:
                return True

        return False

    def get_service_status(self, service, status=None):
        ''' Get service status:
            maintenance : The unit is not yet providing services, but is actively doing stuff.
            unknown : Service has finished an event but the charm has not called status-set yet.
            waiting : Service is unable to progress to an active state because of dependency.
            blocked : Service needs manual intervention to get back to the Running state.
            active  : Service correctly offering all the services.
            None    : Service is not deployed
            *** Make sure this is NOT a asyncio coroutine function ***
        '''
        try:
            self.lock.acquire()
            #self.log.debug ("In get service status for service %s, %s" % (service, services))
            if status is None:
                status = self.get_status()
            if status:
                srv_status = status['Services'][service]['Status']['Status']
                self.lock.release()
                return srv_status
        except KeyError as e:
            self.log.info("Juju: Did not find service {}, e={}".format(service, e))
            self.lock.release()
            return None # Service not deployed
        except Exception as e:
            self.log.error("Juju: exception checking service status for {}, e {}".
                           format(service, e))
            self.reconnect()

        self.lock.release()
        return 'unknown'

    def is_service_active(self, service):
        if self.get_service_status(service) == 'active':
            self.log.debug("Juju: service is active for %s " % service)
            return True

        return False

    def is_service_blocked(self, service):
        if self.get_service_status(service) == 'blocked':
            return True

        return False

    def is_service_up(self, service):
        if self.get_service_status in ['active', 'blocked']:
            return True

        return False

    def wait_for_service(self, service):
        # Check if the agent for the unit is up, wait for units does not wait for service to be up
        # TBD: Should add a timeout, so we do not wait endlessly
        waiting = True
        delay = 5 # seconds
        print ("In wait for service %s" % service)
        while waiting:
             if self.is_service_up(service):
                 return
             else:
                 yield from asyncio.sleep(delay, loop=self.loop)

    @asyncio.coroutine
    def apply_config(self, service, config):
        return self._apply_config(service, config)

    def _apply_config(self, service,config):
        if config is None or len(config) == 0:
            self.log.warn("Juju: Empty config passed for service %s" % service)
            return False
        if not self._is_deployed(service):
            self.log.warn("Juju: Charm service %s not deployed" % (service))
            return False
        self.log.debug("Juju: Config for {} updated to: {}".format(service, config))
        try:
            self.lock.acquire()
            self.env.set_config(service, config)
        except Exception as e:
            self.log.error("Juju: exception setting config for {} with {}, e {}".
                           format(service, config, e))
            self.reconnect()

        self.lock.release()
        return True

    @asyncio.coroutine
    def set_parameter(self, service, parameter, value):
        return self.apply_config(service, {parameter : value})

    @asyncio.coroutine
    def deploy_service(self, charm, service, config=None, wait=False):
        self._deploy_service(charm, service, config=config, wait=wait)

    def _deploy_service(self, charm, service, config=None, wait=False):
        self.log.debug("Juju: Deploy service for charm %s with service %s" %
                       (charm, service))
        if self._is_deployed(service):
            self.log.info("Juju: Charm service %s already deployed" % (service))
            if config:
                self._apply_config(service, config)
            return 'deployed'
        series = "trusty"
        deploy_to = "lxc:0"
        directory = "usr/rift/charms/%s/%s" % (series, charm)
        prefix=''
        try:
            prefix=os.environ.get('RIFT_INSTALL')
        except KeyError:
            self.log.info("Juju: RIFT_INSTALL not set in environemnt")
        directory = "%s/%s" % (prefix, directory)

        try:
            self.lock.acquire()
            self.log.debug("Juju: Local charm settings: dir=%s, series=%s" % (directory, series))
            result = self.env.add_local_charm_dir(directory, series)
            url = result['CharmURL']

        except Exception as e:
            self.log.critical('Juju: Error setting local charm directory {}: {}'.
                              format(service, e))
            self.reconnect()
            self.lock.release()
            return 'error'

        try:
            self.log.debug("Juju: Deploying using: service=%s, url=%s, to=%s, config=%s" %
                           (service, url, deploy_to, config))
            if config:
                self.env.deploy(service, url, machine_spec=deploy_to, config=config)
            else:
                self.env.deploy(service, url, machine_spec=deploy_to)
        except Exception as e:
            self.log.warn('Juju: Error deploying {}: {}'.format(service, e))
            if not self._is_deployed(service):
                self.log.critical ("Juju: Service {} is not deployed" % service)
                self.reconnect()
                self.lock.release()
                return 'error'

        if wait:
            # Wait for the deployed units to start
            try:
                self.log.debug("Juju: Waiting for charm %s to come up" % service)
                self.env.wait_for_units(timeout=self.deploy_timeout)
            except Exception as e:
                self.log.critical('Juju: Error starting all units for {}: {}'.
                                  format(service, e))
                self.reconnect()
                self.lock.release()
                return 'error'

            self.wait_for_service(service)
        self.lock.release()
        return 'deploying'

    @asyncio.coroutine
    def execute_actions(self, service, action, params, wait=False, bail=False):
        return self.execute_actions(service, action, params, wait=wait, bail=bail)

    def _execute_actions(self, service, action, params, wait=False, bail=False):
        try:
            self.lock.acquire()
            services = get_service_units(self.env.status())
            depl_units = services[service]
        except KeyError as e:
            self.log.error("Juju: Unable to get service units for {}, e={}".
                           format(services, e))
            self.lock.release()
            raise e
        except Exception as e:
            self.log.error("Juju: Error on getting service details for service {}, e={}".
                           format(service, e))
            self.reconnect()
            self.lock.release()
            raise e

        tags = []
        # Go through each unit deployed and apply the actions to the unit
        for unit, status in depl_units.items():
            self.log.debug("Juju: Execute on unit %s with %s" % (unit, status))
            idx = int(unit[unit.index('/')+1:])
            self.log.debug("Juju: Unit index is %d" % idx)

            unit_name = "unit-%s-%d" % (service, idx)
            self.log.debug("Juju: Sending action: %s, %s, %s" % (action, unit_name, params))
            try:
                result = self.enqueue_action(action, [unit_name], params)
                if result:
                    tags.append(result.uuid)
                else:
                    self.log.error("Juju: Error applying the action {} on {} with params {}".
                                  format(action, unit, params))
            except Exception as e:
                self.log.error("Juju: Error applying the action {} on {} with params {}, e={}" %
                               format(action, unit, params, e))
                self.reconnect()

            # act_status = 'pending'
            # #self.log.debug("Juju: Action %s status is %s on %s" % (action, act_status, unit))
            # while wait and ((act_status == 'pending') or (act_status == 'running')):
            #     act_status = self.get_action_status(result.uuid)
            #     self.log.debug("Juju: Action %s status is %s on %s" % (action, act_status, unit))
            #     if bail and (act_status ==  'failed'):
            #         self.log.error("Juju: Error applying action %s on %s with %s" % (action, unit, params))
            #         raise RuntimeError("Juju: Error applying action %s on %s with %s" % (action, unit, params))
            #     yield from asyncio.sleep(1, loop=self.loop)

        self.lock.release()
        return tags

    def get_service_units_status(self, service, status):
        units_status = {}
        if status is None:
            return units_status
        try:
            units = get_service_units(status)[service]
            for name, data in units.items():
                # Action rpc require unit name as unit-service-index
                # while resolved API require unit name as service/index
                #idx = int(name[name.index('/')+1:])
                #unit = "unit-%s-%d" % (service, idx)
                units_status.update({name : data['Workload']['Status']})
        except KeyError:
            pass
        except Exception as e:
            self.log.error("Juju: service unit status for service {}, e={}".
                           format(service, e))
        self.log.debug("Juju: service unit status for service {}: {}".
                       format(service, units_status))
        return units_status

    @asyncio.coroutine
    def destroy_service(self, service):
        self._destroy_service(service)

    def _destroy_service(self, service):
        ''' Destroy juju service
            *** Do NOT add aysncio yield on this function, run in separate thread ***
        '''
        self.log.debug("Juju: Destroy charm service: %s" % service)
        try:
            self.lock.acquire()
            status = self.get_status()
        except Exception as e:
            self.log.debug("Juju: Exception getting status e={}".format(e))
            self.reconnect()
        self.lock.release()

        srv_status = self.get_service_status(service, status)
        count = 0
        while srv_status and srv_status not in ['terminated']:
            count += 1
            self.log.debug("Juju: service %s is in %s state, count %d" %
                           (service, srv_status, count))
            if count > 25:
                self.log.error("Juju: Not able to destroy service %s, status %s after %d tries" %
                               (service, srv_status, count))
                break

            units = self.get_service_units_status(service, status)
            for unit, ustatus in units.items():
                if ustatus == 'error':
                    self.log.info("Juju: Found unit %s with status %s" %
                                  (unit, ustatus))
                    try:
                        self.lock.acquire()
                        self.env.resolved(unit)
                    except Exception as e:
                        self.log.debug("Juju: Exception when running resolve ({}) on unit {}: {}".
                                       format(count, unit, e))
                    self.lock.release()

            try:
                self.lock.acquire()
                self.env.destroy_service(service)
            except Exception as e:
                self.log.debug("Juju: Exception when running destroy on service {}: {}".
                       format(unit, e))
            self.lock.release()

            try:
                time.sleep(3)
                self.lock.acquire()
                status = self.get_status()
            except Exception as e:
                self.log.error("Juju: Exception getting status again e={}".format(e))
                self.reconnect()
            self.lock.release()
            srv_status = self.get_service_status(service, status)

        self.log.debug("Destroyed service %s (%s)" % (service, srv_status))
