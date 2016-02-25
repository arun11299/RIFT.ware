
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import asyncio
import logging

from gi.repository import (
    RwDts as rwdts,
    RwIwpYang,
    RwLaunchpadYang,
    RwcalYang as rwcal,
)

import rw_peas
import rift.tasklets


class MissionControlConnectionError(Exception):
    pass


class MissionControlNotConnected(Exception):
    pass


class OutofResourcesError(Exception):
    pass


class PluginLoadingError(Exception):
    pass


def get_add_delete_update_cfgs(dts_member_reg, xact, key_name):
    # Unforunately, it is currently difficult to figure out what has exactly
    # changed in this xact without Pbdelta support (RIFT-4916)
    # As a workaround, we can fetch the pre and post xact elements and
    # perform a comparison to figure out adds/deletes/updates
    xact_cfgs = list(dts_member_reg.get_xact_elements(xact))
    curr_cfgs = list(dts_member_reg.elements)

    xact_key_map = {getattr(cfg, key_name): cfg for cfg in xact_cfgs}
    curr_key_map = {getattr(cfg, key_name): cfg for cfg in curr_cfgs}

    # Find Adds
    added_keys = set(xact_key_map) - set(curr_key_map)
    added_cfgs = [xact_key_map[key] for key in added_keys]

    # Find Deletes
    deleted_keys = set(curr_key_map) - set(xact_key_map)
    deleted_cfgs = [curr_key_map[key] for key in deleted_keys]

    # Find Updates
    updated_keys = set(curr_key_map) & set(xact_key_map)
    updated_cfgs = [xact_key_map[key] for key in updated_keys if xact_key_map[key] != curr_key_map[key]]

    return added_cfgs, deleted_cfgs, updated_cfgs


class ResourcePool(object):
    def __init__(self, log, loop, dts, pool_name, resource_ids):
        self._log = log
        self._loop = loop
        self._dts = dts
        self._pool_name = pool_name
        self._resource_ids = resource_ids

        self._reserved_resource_ids = []

        self._dts_reg = None

    @property
    def pool_xpath(self):
        raise NotImplementedError()

    @property
    def id_field(self):
        raise NotImplementedError()

    def pool_resource_xpath(self, resource_id):
        raise NotImplementedError()

    @asyncio.coroutine
    def reserve_resource(self):
        self._log.debug("Attempting to reserve a resource")

        for id in self._resource_ids:
            self._log.debug("Iterated resource id: %s", id)
            if id not in self._reserved_resource_ids:
                self._log.debug("Reserving resource id %s from pool %s",
                                id, self._pool_name)
                self._reserved_resource_ids.append(id)
                return id

        self._log.warning("Did not find a unreserved resource in pool %s", self._pool_name)
        return None


class VMResourcePool(ResourcePool):
    @property
    def pool_xpath(self):
        return "C,/rw-iwp:resource-mgr/rw-iwp:pools/rw-iwp:vm-pool[rw-iwp:name='{}']/rw-iwp:resources".format(
                self._pool_name,
                )

    @property
    def id_field(self):
        return "vm_id"

    def pool_resource_xpath(self, resource_id):
        return self.pool_xpath + "[rw-iwp:vm-id='{}']".format(
                resource_id,
                )


class NetworkResourcePool(ResourcePool):
    @property
    def pool_xpath(self):
        return "C,/rw-iwp:resource-mgr/rw-iwp:pools/rw-iwp:network-pool[rw-iwp:name='{}']/rw-iwp:resources".format(
                self._pool_name,
                )

    @property
    def id_field(self):
        return "network_id"

    def pool_resource_xpath(self, resource_id):
        return self.pool_xpath + "[rw-iwp:network-id='{}']".format(
                resource_id,
                )


class ResourceManager(object):
    def __init__(self, log, loop, dts):
        self._log = log
        self._loop = loop
        self._dts = dts

        self._resource_mgr_cfg = None

        self._vm_resource_pools = {}
        self._network_resource_pools = {}

        self._periodic_sync_task = None

    @asyncio.coroutine
    def _update_vm_pools(self, vm_pools):
        self._log.debug("Updating vm pools: %s", vm_pools)
        for pool in vm_pools:
            if pool.name not in self._vm_resource_pools:
                self._log.debug("Adding vm resource pool %s", pool.name)
                self._vm_resource_pools[pool.name] = VMResourcePool(
                        self._log,
                        self._loop,
                        self._dts,
                        pool.name,
                        [r.vm_id for r in pool.resources],
                        )

    @asyncio.coroutine
    def _update_network_pools(self, network_pools):
        self._log.debug("Updating network pools: %s", network_pools)
        for pool in network_pools:
            if pool.name not in self._network_resource_pools:
                self._log.debug("Adding network resource pool %s", pool.name)
                self._network_resource_pools[pool.name] = NetworkResourcePool(
                        self._log,
                        self._loop,
                        self._dts,
                        pool.name,
                        [r.network_id for r in pool.resources],
                        )

    @asyncio.coroutine
    def reserve_vm(self):
        self._log.debug("Attempting to reserve a VM resource.")
        for name, pool in self._vm_resource_pools.items():
            resource_id = yield from pool.reserve_resource()
            if resource_id is None:
                continue

            return RwIwpYang.VMResponse(
                    vm_id=resource_id,
                    vm_pool=name,
                    )

        raise OutofResourcesError("Could not find an available network resource")

    @asyncio.coroutine
    def reserve_network(self):
        self._log.debug("Attempting to reserve a Network resource.")
        for name, pool in self._network_resource_pools.items():
            resource_id = yield from pool.reserve_resource()
            if resource_id is None:
                continue

            return RwIwpYang.NetworkResponse(
                    network_id=resource_id,
                    network_pool=name,
                    )

        raise OutofResourcesError("Could not find an available network resource")

    def apply_config(self, resource_mgr_cfg):
        self._log.debug("Applying resource manager config: %s",
                        resource_mgr_cfg)

        self._resource_mgr_cfg = resource_mgr_cfg

        asyncio.ensure_future(
                self._update_network_pools(self._resource_mgr_cfg.pools.network_pool),
                loop=self._loop,
                )

        asyncio.ensure_future(
                self._update_vm_pools(self._resource_mgr_cfg.pools.vm_pool),
                loop=self._loop,
                )


class ResourceRequestHandler(object):
    NETWORK_REQUEST_XPATH = "D,/rw-iwp:resource-mgr/network-request/requests"
    VM_REQUEST_XPATH = "D,/rw-iwp:resource-mgr/vm-request/requests"

    def __init__(self, dts, loop, log, resource_manager, cloud_account):
        self._dts = dts
        self._loop = loop
        self._log = log
        self._resource_manager = resource_manager
        self._cloud_account = cloud_account

        self._network_reg = None
        self._vm_reg = None

        self._network_reg_event = asyncio.Event(loop=self._loop)
        self._vm_reg_event = asyncio.Event(loop=self._loop)

    @asyncio.coroutine
    def wait_ready(self, timeout=5):
        self._log.debug("Waiting for all request registrations to become ready.")
        yield from asyncio.wait(
                [self._network_reg_event.wait(), self._vm_reg_event.wait()],
                timeout=timeout, loop=self._loop,
                )

    def register(self):
        def on_network_request_commit(xact_info):
            """ The transaction has been committed """
            self._log.debug("Got network request commit (xact_info: %s)", xact_info)

            return rwdts.MemberRspCode.ACTION_OK

        @asyncio.coroutine
        def on_request_ready(registration, status):
            self._log.debug("Got request ready event (registration: %s) (status: %s)",
                            registration, status)

            if registration == self._network_reg:
                self._network_reg_event.set()
            elif registration == self._vm_reg:
                self._vm_reg_event.set()
            else:
                self._log.error("Unknown registration ready event: %s", registration)

        @asyncio.coroutine
        def on_network_request_prepare(xact_info, action, ks_path, request_msg):
            self._log.debug(
                    "Got network request on_prepare callback (xact_info: %s, action: %s): %s",
                    xact_info, action, request_msg
                    )

            xpath = ks_path.to_xpath(RwIwpYang.get_schema()) + "/network-response"

            network_item = yield from self._resource_manager.reserve_network()

            network_response = RwIwpYang.NetworkResponse(
                    network_id=network_item.network_id,
                    network_pool=network_item.network_pool
                    )

            self._log.debug("Responding with NetworkResponse at xpath %s: %s",
                            xpath, network_response)
            xact_info.respond_xpath(rwdts.XactRspCode.ACK, xpath, network_response)

        def on_vm_request_commit(xact_info):
            """ The transaction has been committed """
            self._log.debug("Got vm request commit (xact_info: %s)", xact_info)

            return rwdts.MemberRspCode.ACTION_OK

        @asyncio.coroutine
        def on_vm_request_prepare(xact_info, action, ks_path, request_msg):
            def get_vm_ip_address(vm_id):
                rc, vm_info_item = self._cloud_account.cal.get_vm(
                        self._cloud_account.account,
                        vm_id
                        )

                return vm_info_item.management_ip

            self._log.debug(
                    "Got vm request on_prepare callback (xact_info: %s, action: %s): %s",
                    xact_info, action, request_msg
                    )

            xpath = ks_path.to_xpath(RwIwpYang.get_schema()) + "/vm-response"

            vm_item = yield from self._resource_manager.reserve_vm()

            vm_ip = get_vm_ip_address(vm_item.vm_id)

            vm_response = RwIwpYang.VMResponse(
                    vm_id=vm_item.vm_id,
                    vm_pool=vm_item.vm_pool,
                    vm_ip=vm_ip,
                    )

            self._log.debug("Responding with VMResponse at xpath %s: %s",
                            xpath, vm_response)
            xact_info.respond_xpath(rwdts.XactRspCode.ACK, xpath, vm_response)

        with self._dts.group_create() as group:
            self._log.debug("Registering for Network Resource Request using xpath: %s",
                            ResourceRequestHandler.NETWORK_REQUEST_XPATH,
                            )

            self._network_reg = group.register(
                    xpath=ResourceRequestHandler.NETWORK_REQUEST_XPATH,
                    handler=rift.tasklets.DTS.RegistrationHandler(
                        on_ready=on_request_ready,
                        on_commit=on_network_request_commit,
                        on_prepare=on_network_request_prepare,
                        ),
                    flags=rwdts.Flag.PUBLISHER,
                    )

            self._log.debug("Registering for VM Resource Request using xpath: %s",
                            ResourceRequestHandler.VM_REQUEST_XPATH,
                            )
            self._vm_reg = group.register(
                    xpath=ResourceRequestHandler.VM_REQUEST_XPATH,
                    handler=rift.tasklets.DTS.RegistrationHandler(
                        on_ready=on_request_ready,
                        on_commit=on_vm_request_commit,
                        on_prepare=on_vm_request_prepare,
                        ),
                    flags=rwdts.Flag.PUBLISHER,
                    )


class ResourceMgrDtsConfigHandler(object):
    XPATH = "C,/rw-iwp:resource-mgr"

    def __init__(self, dts, log, resource_manager):
        self._dts = dts
        self._log = log

        self._resource_manager = resource_manager
        self._res_mgr_cfg = RwIwpYang.ResourceManagerConfig()

    def register(self):
        def on_apply(dts, acg, xact, action, _):
            """Apply the resource manager configuration"""

            if xact.xact is None:
                # When RIFT first comes up, an INSTALL is called with the current config
                # Since confd doesn't actally persist data this never has any data so
                # skip this for now.
                self._log.debug("No xact handle.  Skipping apply config")
                return

            self._log.debug("Got resource mgr apply config (xact: %s) (action: %s)",
                            xact, action)

            self._resource_manager.apply_config(self._res_mgr_cfg)

        @asyncio.coroutine
        def on_prepare(dts, acg, xact, xact_info, ks_path, msg):
            self._log.debug("Got resource manager configuration: %s", msg)

            mgmt_domain = msg.mgmt_domain
            if mgmt_domain.has_field("name"):
                self._res_mgr_cfg.mgmt_domain.name = mgmt_domain.name

            mission_control = msg.mission_control
            if mission_control.has_field("mgmt_ip"):
                self._res_mgr_cfg.mission_control.mgmt_ip = mission_control.mgmt_ip

            if msg.has_field("pools"):
                self._res_mgr_cfg.pools.from_dict(msg.pools.as_dict())

            acg.handle.prepare_complete_ok(xact_info.handle)

        self._log.debug("Registering for Resource Mgr config using xpath: %s",
                        ResourceMgrDtsConfigHandler.XPATH,
                        )

        acg_handler = rift.tasklets.AppConfGroup.Handler(on_apply=on_apply)
        with self._dts.appconf_group_create(handler=acg_handler) as acg:
            self._pool_reg = acg.register(
                    xpath=ResourceMgrDtsConfigHandler.XPATH,
                    flags=rwdts.Flag.SUBSCRIBER,
                    on_prepare=on_prepare
                    )


class CloudAccountDtsHandler(object):
    XPATH = "C,/rw-launchpad:cloud-account"
    log_hdl = None

    def __init__(self, dts, log, cal_account):
        self.dts = dts
        self.log = log
        self.cal_account = cal_account
        self.reg = None

    def add_account(self, account):
        self.log.info("adding cloud account: {}".format(account))
        self.cal_account.account = rwcal.CloudAccount.from_dict(account.as_dict())
        self.cal_account.cal = self.load_cal_plugin(account)

    def delete_account(self, account_id):
        self.log.info("deleting cloud account: {}".format(account_id))
        self.cal_account.account = None
        self.cal_account.cal = None

    def update_account(self, account):
        self.log.info("updating cloud account: {}".format(account))
        self.cal_account.account = rwcal.CloudAccount.from_dict(account.as_dict())
        self.cal_account.cal = self.load_cal_plugin(account)

    def load_cal_plugin(self, account):
        try:
            plugin = rw_peas.PeasPlugin(
                    getattr(account, account.account_type).plugin_name,
                    'RwCal-1.0'
                    )

        except AttributeError as e:
            raise PluginLoadingError(str(e))

        engine, info, ext = plugin()

        # Initialize the CAL interface
        cal = plugin.get_interface("Cloud")
        cal.init(CloudAccountDtsHandler.log_hdl)

        return cal

    @asyncio.coroutine
    def register(self):
        def apply_config(dts, acg, xact, action, _):
            self.log.debug("Got cloud account apply config (xact: %s) (action: %s)", xact, action)

            if xact.xact is None:
                # When RIFT first comes up, an INSTALL is called with the current config
                # Since confd doesn't actally persist data this never has any data so
                # skip this for now.
                self.log.debug("No xact handle.  Skipping apply config")
                return

            add_cfgs, delete_cfgs, update_cfgs = get_add_delete_update_cfgs(
                    dts_member_reg=self.reg,
                    xact=xact,
                    key_name="name",
                    )

            # Handle Deletes
            for cfg in delete_cfgs:
                self.delete_account(cfg.name)

            # Handle Adds
            for cfg in add_cfgs:
                self.add_account(cfg)

            # Handle Updates
            for cfg in update_cfgs:
                self.update_account(cfg)

        self.log.debug("Registering for Cloud Account config using xpath: %s",
                        CloudAccountDtsHandler.XPATH,
                        )

        acg_handler = rift.tasklets.AppConfGroup.Handler(
                        on_apply=apply_config,
                        )

        with self.dts.appconf_group_create(acg_handler) as acg:
            self.reg = acg.register(
                    xpath=CloudAccountDtsHandler.XPATH,
                    flags=rwdts.Flag.SUBSCRIBER,
                    )


class CloudAccount(object):
    def __init__(self):
        self.cal = None
        self.account = None


class IwpTasklet(rift.tasklets.Tasklet):
    def __init__(self, *args, **kwargs):
        super(IwpTasklet, self).__init__(*args, **kwargs)

        self._dts = None

        self._resource_manager = None
        self._resource_mgr_config_hdl = None

        self._cloud_account = CloudAccount()

    def start(self):
        super(IwpTasklet, self).start()
        self.log.info("Starting IwpTasklet")
        self.log.setLevel(logging.DEBUG)

        self.log.debug("Registering with dts")
        self._dts = rift.tasklets.DTS(
                self.tasklet_info,
                RwLaunchpadYang.get_schema(),
                self.loop,
                self.on_dts_state_change
                )

        CloudAccountDtsHandler.log_hdl = self.log_hdl

        self.log.debug("Created DTS Api GI Object: %s", self._dts)

    def on_instance_started(self):
        self.log.debug("Got instance started callback")

    @asyncio.coroutine
    def init(self):
        self._resource_manager = ResourceManager(
                self._log,
                self._loop,
                self._dts
                )

        self.log.debug("creating resource mgr config request handler")
        self._resource_mgr_config_hdl = ResourceMgrDtsConfigHandler(
                self._dts,
                self.log,
                self._resource_manager,
                )
        self._resource_mgr_config_hdl.register()

        self.log.debug("creating resource request handler")
        self._resource_req_hdl = ResourceRequestHandler(
                self._dts,
                self.loop,
                self.log,
                self._resource_manager,
                self._cloud_account,
                )
        self._resource_req_hdl.register()

        self.log.debug("creating cloud account handler")
        self.account_handler = CloudAccountDtsHandler(self._dts, self.log, self._cloud_account)
        yield from self.account_handler.register()

    @asyncio.coroutine
    def run(self):
        pass

    @asyncio.coroutine
    def on_dts_state_change(self, state):
        """Take action according to current dts state to transition
        application into the corresponding application state

        Arguments
            state - current dts state
        """
        switch = {
            rwdts.State.INIT: rwdts.State.REGN_COMPLETE,
            rwdts.State.CONFIG: rwdts.State.RUN,
        }

        handlers = {
            rwdts.State.INIT: self.init,
            rwdts.State.RUN: self.run,
        }

        # Transition application to next state
        handler = handlers.get(state, None)
        if handler is not None:
            yield from handler()

        # Transition dts to next state
        next_state = switch.get(state, None)
        if next_state is not None:
            self._dts.handle.set_state(next_state)
