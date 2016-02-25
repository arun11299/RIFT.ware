# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import asyncio
import rift.tasklets

from . import rwnsmconfigplugin
from . import jujuconf_nsm
import rift.mano.config_agent

class ConfigAgentExistsError(Exception):
    pass

class ConfigAccountHandler(object):
    def __init__(self, dts, log, loop, on_add_config_agent):
        self._log = log
        self._dts = dts
        self._loop = loop
        self._on_add_config_agent = on_add_config_agent

        self._log.debug("creating config account handler")
        self.cloud_cfg_handler = rift.mano.config_agent.ConfigAgentSubscriber(
            self._dts, self._log,
            rift.mano.config_agent.ConfigAgentCallbacks(
                on_add_apply=self.on_config_account_added,
                on_delete_apply=self.on_config_account_deleted,
                on_update_prepare=self.on_config_account_update,
            )
        )

    def on_config_account_deleted(self, account_name):
        self._log.debug("config account deleted")
        self._log.debug(account_name)
        self._log.error("Config agent update not supported yet")

    def on_config_account_added(self, account):
        self._log.debug("config account added")
        self._log.debug(account.as_dict())
        self._on_add_config_agent(account)

    @asyncio.coroutine
    def on_config_account_update(self, account):
        self._log.debug("config account being updated")
        self._log.debug(account.as_dict())
        self._log.error("Config agent update not supported yet")

    @asyncio.coroutine
    def register(self):
        self.cloud_cfg_handler.register()

class RwNsConfigPlugin(rwnsmconfigplugin.NsmConfigPluginBase):
    """
        Default Implementation of the NsmConfPluginBase
    """
    @asyncio.coroutine
    def notify_create_nsr(self, nsr, nsd):
        """
        Notification of create Network service record
        """
        pass

    @asyncio.coroutine
    def apply_config(self, config, nsr, vnfrs):
        """
        Notification of configuration of Network service record
        """
        pass

    @asyncio.coroutine
    def notify_create_vls(self, nsr, vld):
        """
        Notification of create Network service record
        """
        pass

    @asyncio.coroutine
    def notify_create_vnfr(self, nsr, vnfr):
        """
        Notification of create Network service record
        """
        pass

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
        pass

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
        pass

    @asyncio.coroutine
    def notify_terminate_vl(self, nsr, vlr, xact):
        """
        Notification of Terminate the virtual link
        """
        pass

    @asyncio.coroutine
    def apply_initial_config(self, vnfr_id, vnf):
        """Apply initial configuration"""
        pass

    @asyncio.coroutine
    def get_config_status(self, vnfr_id):
        """Get the status for the VNF"""
        pass

    def get_action_status(self, execution_id):
        """Get the action exection status"""
        pass

    @asyncio.coroutine
    def vnf_config_primitive(self, nsr_id, vnfr_id, primitive, output):
        """Apply config primitive on a VNF"""
        pass

class NsmConfigPlugins(object):
    """ NSM Config Agent Plugins """
    def __init__(self):
        self._plugin_classes = {
                "juju": jujuconf_nsm.JujuNsmConfigPlugin,
                }

    @property
    def plugins(self):
        """ Plugin info """
        return self._plugin_classes

    def __getitem__(self, name):
        """ Get item """
        print("%s", self._plugin_classes)
        return self._plugin_classes[name]

    def register(self, plugin_name, plugin_class, *args):
        """ Register a plugin to this Nsm"""
        self._plugin_classes[plugin_name] = plugin_class

    def deregister(self, plugin_name, plugin_class, *args):
        """ Deregister a plugin to this Nsm"""
        if plugin_name in self._plugin_classes:
            del self._plugin_classes[plugin_name]

    def class_by_plugin_name(self, name):
        """ Get class by plugin name """
        return self._plugin_classes[name]


class NsmConfigAgent(object):
    def __init__(self, dts, log, loop, records_publisher, on_config_nsm_plugin):
        self._dts = dts
        self._log = log
        self._loop = loop

        self._records_publisher = records_publisher
        self._on_config_nsm_plugin = on_config_nsm_plugin
        self._config_plugins = NsmConfigPlugins()
        self._config_handler = ConfigAccountHandler(
            self._dts, self._log, self._loop, self._on_config_agent)
        self._plugin_instances = {}

    def _set_plugin_instance(self, instance):
        self._on_config_nsm_plugin(instance)

    def _on_config_agent(self, config_agent):
        self._log.debug("Got nsm plugin config agent account: %s", config_agent)
        try:
            nsm_cls = self._config_plugins.class_by_plugin_name(
                config_agent.account_type)
        except KeyError as e:
            self._log.debug(
                "Config agent nsm plugin type not found: {}.  Using default plugin, e={}".
                format(config_agent.account_type, e))
            nsm_cls = RwNsConfigPlugin

        # Check to see if the plugin was already instantiated
        if nsm_cls in self._plugin_instances:
            self._log.debug("Config agent nsm plugin already instantiated.  Using existing.")
            self._set_plugin_instance(self._plugin_instances[nsm_cls])

        # Otherwise, instantiate a new plugin using the config agent account
        self._log.debug("Instantiting new config agent using class: %s", nsm_cls)
        nsm_instance = nsm_cls(self._dts, self._log, self._loop, self._records_publisher, config_agent)
        self._plugin_instances[nsm_cls] = nsm_instance

        self._set_plugin_instance(self._plugin_instances[nsm_cls])

    @asyncio.coroutine
    def register(self):
        self._log.debug("Registering for config agent nsm plugin manager")
        yield from self._config_handler.register()
