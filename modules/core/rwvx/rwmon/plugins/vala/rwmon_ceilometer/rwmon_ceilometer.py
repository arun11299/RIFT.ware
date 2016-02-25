
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import logging

from gi.repository import (
    GObject,
    RwMon,
    RwTypes,
    RwcalYang,
    RwmonYang,
    )

import rift.rwcal.openstack as openstack_drv
import rw_status
import rwlogger

logger = logging.getLogger('rwmon.ceilometer')

rwstatus = rw_status.rwstatus_from_exc_map({
    IndexError: RwTypes.RwStatus.NOTFOUND,
    KeyError: RwTypes.RwStatus.NOTFOUND,
    })

class CeilometerMonitoringPlugin(GObject.Object, RwMon.Monitoring):
    def __init__(self):
        GObject.Object.__init__(self)
        self._driver_class = openstack_drv.OpenstackDriver

    def _get_driver(self, account):
        return self._driver_class(username = account.openstack.key,
                                  password = account.openstack.secret,
                                  auth_url = account.openstack.auth_url,
                                  tenant_name = account.openstack.tenant,
                                  mgmt_network = account.openstack.mgmt_network)

    @rwstatus
    def do_init(self, rwlog_ctx):
        if not any(isinstance(h, rwlogger.RwLogger) for h in logger.handlers):
            logger.addHandler(
                rwlogger.RwLogger(
                    category="rwmon.ceilometer",
                    log_hdl=rwlog_ctx,
                )
            )

    @rwstatus(ret_on_failure=[None])
    def do_nfvi_metrics(self, account, vmid):
        try:
            samples = self._get_driver(account).ceilo_nfvi_metrics(vmid)

            metrics = RwmonYang.NfviMetrics()

            metrics.vcpu.utilization = samples.get("cpu_util", 0)
            metrics.memory.used = samples.get("memory_usage", 0)
            metrics.storage.used = samples.get("disk_usage", 0)

            return metrics

        except Exception as e:
            logger.exception(e)

    @rwstatus(ret_on_failure=[None])
    def do_nfvi_metrics_available(self, account):
        endpoint = self._get_driver(account).ceilo_meter_endpoint()
        return endpoint is not None
