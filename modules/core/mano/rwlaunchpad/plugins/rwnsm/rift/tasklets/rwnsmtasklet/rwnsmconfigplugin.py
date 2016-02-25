# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import asyncio
import abc


class NsmConfigPluginBase(object):
    """
        Abstract base class for the NSM Configuration agent plugin.
        There will be single instance of this plugin for each plugin type.
    """

    def __init__(self, dts, log, loop, publisher, config_agent):
        self._dts = dts
        self._log = log
        self._loop = loop
        self._publisher = publisher
        self._config_agent = config_agent

    @property
    def dts(self):
        return self._dts

    @property
    def log(self):
        return self._log

    @property
    def loop(self):
        return self._loop

    @property
    def nsm(self):
        return self._nsm


    @abc.abstractmethod
    @asyncio.coroutine
    def notify_create_nsr(self, nsr, nsd):
        """ Notification on creation of an NSR """
        pass

    @abc.abstractmethod
    @asyncio.coroutine
    def apply_config(self, config, nsrs, vnfrs):
        """ Notification on configuration of an NSR """
        pass

    @abc.abstractmethod
    @asyncio.coroutine
    def notify_create_vls(self, nsr, vld):
        """ Notification on creation of an VL """
        pass

    @abc.abstractmethod
    @asyncio.coroutine
    def notify_create_vnfr(self, nsr, vnfr):
        """ Notification on creation of an VNFR """
        pass

    @abc.abstractmethod
    @asyncio.coroutine
    def notify_instantiate_ns(self, nsr):
        """ Notification for instantiate of the network service """
        pass

    @abc.abstractmethod
    @asyncio.coroutine
    def notify_instantiate_vnf(self, nsr, vnfr, xact):
        """ Notify instantiation of the virtual network function """
        pass

    @abc.abstractmethod
    @asyncio.coroutine
    def notify_instantiate_vl(self, nsr, vl, xact):
        """ Notify instantiate of the virtual link"""
        pass

    @abc.abstractmethod
    @asyncio.coroutine
    def notify_nsr_active(self, nsr, vnfrs):
        """ Notify instantiate of the virtual link"""
        pass

    @abc.abstractmethod
    @asyncio.coroutine
    def notify_terminate_ns(self, nsr):
        """Notify termination of the network service """
        pass

    @abc.abstractmethod
    @asyncio.coroutine
    def notify_terminate_vnf(self, nsr, vnfr, xact):
        """Notify termination of the VNF """
        pass

    @abc.abstractmethod
    @asyncio.coroutine
    def notify_terminate_vl(self, nsr, vlr, xact):
        """Notify termination of the Virtual Link Record"""
        pass

    @abc.abstractmethod
    @asyncio.coroutine
    def apply_initial_config(self, vnfr_id, vnf):
        """Apply initial configuration"""
        pass

    @abc.abstractmethod
    @asyncio.coroutine
    def get_config_status(self, vnfr_id):
        """Get the status for the VNF"""
        pass

    @abc.abstractmethod
    def get_action_status(self, execution_id):
        """Get the action exection status"""
        pass

    @abc.abstractmethod
    @asyncio.coroutine
    def is_configured(self, vnfr_if):
        """ Check if the agent is configured for the VNFR """
        pass

    @abc.abstractmethod
    @asyncio.coroutine
    def vnf_config_primitive(self, nsr_id, vnfr_id, primitive, output):
        """Apply config primitive on a VNF"""

    @asyncio.coroutine
    def invoke(self, method, *args):
        self._log.debug("Config agent plugin: method %s with args %s: %s" % (method, args, self))
        # TBD - Do a better way than string compare to find invoke the method
        if method == 'notify_create_nsr':
            yield from self.notify_create_nsr(args[0], args[1])
        elif method == 'notify_create_vls':
            yield from self.notify_create_vls(args[0], args[1], args[2])
        elif method == 'notify_create_vnfr':
            yield from self.notify_create_vnfr(args[0], args[1])
        elif method == 'notify_instantiate_ns':
            yield from self.notify_instantiate_ns(args[0])
        elif method == 'notify_instantiate_vnf':
            yield from self.notify_instantiate_vnf(args[0], args[1], args[2])
        elif method == 'notify_instantiate_vl':
            yield from self.notify_instantiate_vl(args[0], args[1], args[2])
        elif method == 'notify_nsr_active':
            yield from self.notify_nsr_active(args[0], args[1])
        elif method == 'notify_terminate_ns':
            yield from self.notify_terminate_ns(args[0])
        elif method == 'notify_terminate_vnf':
            yield from self.notify_terminate_vnf(args[0], args[1], args[2])
        elif method == 'notify_terminate_vl':
            yield from self.notify_terminate_vl(args[0], args[1], args[2])
        elif method == 'apply_initial_config':
            yield from self.apply_initial_config(args[0], args[1])
        elif method == 'apply_config':
            yield from self.apply_config(args[0], args[1], args[2])
        else:
            self._log.error("Unknown method %s invoked on config agent plugin" % method)
