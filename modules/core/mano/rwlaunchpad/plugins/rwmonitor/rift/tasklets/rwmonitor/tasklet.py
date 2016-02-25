
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import asyncio
import collections
import concurrent.futures
import os
import time
import uuid

from gi.repository import (
    NsrYang,
    RwBaseYang,
    RwCompositeYang,
    RwDts as rwdts,
    RwLaunchpadYang,
    RwLog as rwlog,
    RwcalYang as rwcal,
    RwMonitorYang as rwmonitor,
    RwmonYang as rwmon,
    RwNsdYang as rwnsd,
    RwTypes,
    RwYang,
    VnfrYang,
)

import rift.tasklets
import rift.mano.cloud

import rw_peas

from .core import (NfviMetricsAggregator, RecordManager)


class DtsHandler(object):
    def __init__(self, tasklet):
        self.reg = None
        self.tasklet = tasklet

    @property
    def log(self):
        return self.tasklet.log

    @property
    def log_hdl(self):
        return self.tasklet.log_hdl

    @property
    def dts(self):
        return self.tasklet.dts

    @property
    def loop(self):
        return self.tasklet.loop

    @property
    def classname(self):
        return self.__class__.__name__


class NsInstanceOpdataSubscriber(DtsHandler):
    XPATH = "D,/nsr:ns-instance-opdata/nsr:nsr"

    @asyncio.coroutine
    def register(self):
        def handle_create(msg):
            self.tasklet.records.add_nsr(msg)
            self.tasklet.start_ns_monitor(msg)

        def handle_update(msg):
            self.tasklet.records.add_nsr(msg)

        def handle_delete(msg):
            self.tasklet.records.remove_nsr(msg.ns_instance_config_ref)

        def ignore(msg):
            pass

        dispatch = {
                rwdts.QueryAction.CREATE: handle_create,
                rwdts.QueryAction.UPDATE: handle_update,
                rwdts.QueryAction.DELETE: handle_delete,
                }

        @asyncio.coroutine
        def on_prepare(xact_info, action, ks_path, msg):
            try:
                # Disabling the following comments since they are too frequent
                # self.log.debug("{}:on_prepare:msg {}".format(self.classname, msg))

                if msg is not None:
                    dispatch.get(action, ignore)(msg)

            except Exception as e:
                self.log.exception(e)

            finally:
                # Disabling the following comments since they are too frequent
                # self.log.debug("{}:on_prepare complete".format(self.classname))
                xact_info.respond_xpath(rwdts.XactRspCode.ACK)

        handler = rift.tasklets.DTS.RegistrationHandler(
                on_prepare=on_prepare,
                )

        with self.dts.group_create() as group:
            group.register(
                    xpath=NsInstanceOpdataSubscriber.XPATH,
                    flags=rwdts.Flag.SUBSCRIBER,
                    handler=handler,
                    )


class VnfrCatalogSubscriber(DtsHandler):
    XPATH = "D,/vnfr:vnfr-catalog/vnfr:vnfr"

    @asyncio.coroutine
    def register(self):
        def handle_create(msg):
            self.log.debug("{}:handle_create:{}".format(self.classname, msg))
            self.tasklet.records.add_vnfr(msg)

        def handle_update(msg):
            self.log.debug("{}:handle_update:{}".format(self.classname, msg))
            self.tasklet.records.add_vnfr(msg)

        def handle_delete(msg):
            self.tasklet.records.remove_vnfr(msg)

        def ignore(msg):
            pass

        dispatch = {
                rwdts.QueryAction.CREATE: handle_create,
                rwdts.QueryAction.UPDATE: handle_update,
                rwdts.QueryAction.DELETE: handle_delete,
                }

        @asyncio.coroutine
        def on_prepare(xact_info, action, ks_path, msg):
            try:
                self.log.debug("{}:on_prepare".format(self.classname))
                self.log.debug("{}:on_preparef:msg {}".format(self.classname, msg))

                xpath = ks_path.to_xpath(VnfrYang.get_schema())
                xact_info.respond_xpath(rwdts.XactRspCode.ACK, xpath)

                dispatch.get(action, ignore)(msg)

            except Exception as e:
                self.log.exception(e)

            finally:
                self.log.debug("{}:on_prepare complete".format(self.classname))

        handler = rift.tasklets.DTS.RegistrationHandler(
                on_prepare=on_prepare,
                )

        with self.dts.group_create() as group:
            group.register(
                    xpath=VnfrCatalogSubscriber.XPATH,
                    flags=rwdts.Flag.SUBSCRIBER,
                    handler=handler,
                    )


class NfviPollingPeriodSubscriber(DtsHandler):
    XPATH = "C,/nsr:ns-instance-config"

    @asyncio.coroutine
    def register(self):
        def on_apply(dts, acg, xact, action, _):
            if xact.xact is None:
                # When RIFT first comes up, an INSTALL is called with the current config
                # Since confd doesn't actally persist data this never has any data so
                # skip this for now.
                self.log.debug("No xact handle. Skipping apply config")
                return

            xact_config = list(self.reg.get_xact_elements(xact))
            for config in xact_config:
                if config.nfvi_polling_period is not None:
                    self.tasklet.polling_period = config.nfvi_polling_period
                    self.log.debug("new polling period: {}".format(self.tasklet.polling_period))

        self.log.debug(
                "Registering for NFVI polling period config using xpath: %s",
                NfviPollingPeriodSubscriber.XPATH,
                )

        acg_handler = rift.tasklets.AppConfGroup.Handler(
                        on_apply=on_apply,
                        )

        with self.dts.appconf_group_create(acg_handler) as acg:
            self.reg = acg.register(
                    xpath=NfviPollingPeriodSubscriber.XPATH,
                    flags=rwdts.Flag.SUBSCRIBER,
                    )


class CloudAccountDtsHandler(DtsHandler):
    def __init__(self, tasklet):
        super().__init__(tasklet)
        self._cloud_cfg_subscriber = None

    def on_account_added_apply(self, account):
        self.log.info("adding cloud account: {}".format(account))
        self.tasklet.cloud_accounts[account.name] = account.cal_account_msg
        self.tasklet.account_nfvi_monitors[account.name] = self.load_nfvi_monitor_plugin(account.cal_account_msg)

    def on_account_deleted_apply(self, account_name):
        self.log.info("deleting cloud account: {}".format(account_name))
        if account_name in self.tasklet.cloud_accounts:
            del self.tasklet.cloud_accounts[account_name]

        if account_name in self.tasklet.account_nfvi_monitors:
            del self.tasklet.account_nfvi_monitors[account_name]

    @asyncio.coroutine
    def on_account_updated_prepare(self, account):
        raise NotImplementedError("Monitor does not support updating cloud account")

    def load_nfvi_monitor_plugin(self, cloud_account):
        if cloud_account.account_type == "openstack":
            self.log.debug('loading ceilometer plugin for NFVI metrics')
            plugin = rw_peas.PeasPlugin(
                    "rwmon_ceilometer",
                    'RwMon-1.0',
                    )

        else:
            self.log.debug('loading mock plugin for NFVI metrics')
            plugin = rw_peas.PeasPlugin(
                    "rwmon_mock",
                    'RwMon-1.0',
                    )

        impl = plugin.get_interface("Monitoring")
        impl.init(self.log_hdl)

        # Check that the plugin is available on this platform
        _, available = impl.nfvi_metrics_available(cloud_account)
        if not available:
            self.log.warning('NFVI monitoring unavailable on this host')
            return None

        return impl

    def register(self):
        self.log.debug("creating cloud account config handler")
        self._cloud_cfg_subscriber = rift.mano.cloud.CloudAccountConfigSubscriber(
               self.dts, self.log, self.log_hdl,
               rift.mano.cloud.CloudAccountConfigCallbacks(
                   on_add_apply=self.on_account_added_apply,
                   on_delete_apply=self.on_account_deleted_apply,
                   on_update_prepare=self.on_account_updated_prepare,
               )
           )
        self._cloud_cfg_subscriber.register()


class MonitorTasklet(rift.tasklets.Tasklet):
    """
    The MonitorTasklet is responsible for sampling NFVI mettrics (via a CAL
    plugin) and publishing the aggregate information.
    """

    DEFAULT_POLLING_PERIOD = 1.0

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.nsr_subscriber = NsInstanceOpdataSubscriber(self)
        self.vnfr_subscriber = VnfrCatalogSubscriber(self)
        self.cloud_cfg_subscriber = CloudAccountDtsHandler(self)
        self.poll_period_subscriber = NfviPollingPeriodSubscriber(self)
        self.cloud_account_handler = CloudAccountDtsHandler(self)

        self.vnfrs = collections.defaultdict(list)
        self.vdurs = collections.defaultdict(list)

        self.monitors = dict()
        self.cloud_accounts = {}
        self.account_nfvi_monitors = {}

        self.records = RecordManager()
        self.polling_period = MonitorTasklet.DEFAULT_POLLING_PERIOD
        self.executor = concurrent.futures.ThreadPoolExecutor(max_workers=16)

    def start(self):
        super().start()
        self.log.info("Starting MonitoringTasklet")

        self.log.debug("Registering with dts")
        self.dts = rift.tasklets.DTS(
                self.tasklet_info,
                rwmonitor.get_schema(),
                self.loop,
                self.on_dts_state_change
                )

        self.log.debug("Created DTS Api GI Object: %s", self.dts)

    @asyncio.coroutine
    def init(self):
        self.log.debug("creating cloud account handler")
        self.cloud_cfg_subscriber.register()

        self.log.debug("creating NFVI poll period subscriber")
        yield from  self.poll_period_subscriber.register()

        self.log.debug("creating network service record subscriber")
        yield from self.nsr_subscriber.register()

        self.log.debug("creating vnfr subscriber")
        yield from self.vnfr_subscriber.register()

    def on_cloud_account_created(self, cloud_account):
        pass

    def on_cloud_account_deleted(self, cloud_account):
        pass

    @asyncio.coroutine
    def run(self):
        pass

    def on_instance_started(self):
        self.log.debug("Got instance started callback")

    @asyncio.coroutine
    def on_dts_state_change(self, state):
        """Handle DTS state change

        Take action according to current DTS state to transition application
        into the corresponding application state

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
            self.dts.handle.set_state(next_state)

    def start_ns_monitor(self, ns_instance_opdata_msg):
        ns_instance_config_ref = ns_instance_opdata_msg.ns_instance_config_ref
        nsr_cloud_account = ns_instance_opdata_msg.cloud_account

        if nsr_cloud_account not in self.cloud_accounts:
            self.log.error("cloud account %s has not been configured", nsr_cloud_account)
            return

        if nsr_cloud_account not in self.account_nfvi_monitors:
            self.log.warning("No NFVI monitoring available for cloud account %s",
                             nsr_cloud_account)
            return

        cloud_account = self.cloud_accounts[nsr_cloud_account]
        nfvi_monitor = self.account_nfvi_monitors[nsr_cloud_account]

        try:
            if ns_instance_config_ref not in self.monitors:
                aggregator = NfviMetricsAggregator(
                        tasklet=self,
                        cloud_account=cloud_account,
                        nfvi_monitor=nfvi_monitor,
                        )

                # Create a task to run the aggregator independently
                coro = aggregator.publish_nfvi_metrics(ns_instance_config_ref)
                task = self.loop.create_task(coro)
                self.monitors[ns_instance_config_ref] = task

                msg = 'started monitoring NFVI metrics for {}'
                self.log.info(msg.format(ns_instance_config_ref))

        except Exception as e:
            self.log.exception(e)
            raise

    def stop_ns_monitor(self, ns_instance_config_ref):
        if ns_instance_config_ref not in self.monitors:
            msg = "Trying the destroy non-existent monitor for {}"
            self.log.error(msg.format(ns_instance_config_ref))

        else:
            self.monitors[ns_instance_config_ref].cancel()
            del self.monitors[ns_instance_config_ref]
