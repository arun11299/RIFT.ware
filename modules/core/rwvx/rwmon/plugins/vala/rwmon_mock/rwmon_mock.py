
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import logging

from gi.repository import (
    GObject,
    RwMon,
    RwTypes,
    RwmonYang as rwmon,
    )

import rw_status
import rwlogger

logger = logging.getLogger('rwmon.mock')


rwstatus = rw_status.rwstatus_from_exc_map({
    IndexError: RwTypes.RwStatus.NOTFOUND,
    KeyError: RwTypes.RwStatus.NOTFOUND,
    })


class NullImpl(object):
    def nfvi_metrics(self, account, vm_id):
        return rwmon.NfviMetrics()

    def nfvi_metrics_available(self, account):
        return True


class MockMonitoringPlugin(GObject.Object, RwMon.Monitoring):
    def __init__(self):
        GObject.Object.__init__(self)
        self._impl = NullImpl()

    @rwstatus
    def do_init(self, rwlog_ctx):
        if not any(isinstance(h, rwlogger.RwLogger) for h in logger.handlers):
            logger.addHandler(
                rwlogger.RwLogger(
                    category="rwmon.mock",
                    log_hdl=rwlog_ctx,
                )
            )

    @rwstatus
    def do_nfvi_metrics(self, account, vm_id):
        return self._impl.nfvi_metrics(account, vm_id)

    @rwstatus
    def do_nfvi_metrics_available(self, account):
        return self._impl.nfvi_metrics_available(account)

    def set_impl(self, impl):
        self._impl = impl
