#!/usr/bin/env python3

# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#


import argparse
import asyncio
import concurrent.futures
import logging
import os
import sys
import unittest
import uuid
import xmlrunner

from gi.repository import (
        RwcalYang,
        RwVnfrYang,
        RwTypes,
        VnfrYang,
        NsrYang,
        )

from rift.tasklets.rwmonitor.core import (RecordManager, NfviMetricsAggregator)


class MockTasklet(object):
    def __init__(self, dts, log, loop, records):
        self.dts = dts
        self.log = log
        self.loop = loop
        self.records = records
        self.polling_period = 0
        self.executor = concurrent.futures.ThreadPoolExecutor(max_workers=16)


def make_nsr(ns_instance_config_ref=str(uuid.uuid4())):
    nsr = NsrYang.YangData_Nsr_NsInstanceOpdata_Nsr()
    nsr.ns_instance_config_ref = ns_instance_config_ref
    return nsr

def make_vnfr(id=str(uuid.uuid4())):
    vnfr = VnfrYang.YangData_Vnfr_VnfrCatalog_Vnfr()
    vnfr.id = id
    return vnfr

def make_vdur(id=str(uuid.uuid4()), vim_id=str(uuid.uuid4())):
    vdur = VnfrYang.YangData_Vnfr_VnfrCatalog_Vnfr_Vdur()
    vdur.id = id
    vdur.vim_id = vim_id
    return vdur


class MockNfviMonitorPlugin(object):
    def __init__(self):
        self.metrics = dict()

    def nfvi_metrics(self, account, vim_id):
        key = (account, vim_id)

        if key in self.metrics:
            return RwTypes.RwStatus.SUCCESS, self.metrics[key]

        metrics = RwVnfrYang.YangData_Vnfr_VnfrCatalog_Vnfr_Vdur_NfviMetrics()
        status = RwTypes.RwStatus.FAILURE

        return status, metrics


class TestAggregator(unittest.TestCase):
    """
    Ths NfviMetricsAggregator queries NFVI metrics from VIM components and
    aggregates the data are the VNF and NS levels. This test case validates
    that the aggregation happens as expected.
    """

    def setUp(self):
        self.nfvi_monitor = MockNfviMonitorPlugin()
        self.cloud_account = RwcalYang.CloudAccount(
                    name="test-account",
                    account_type="mock",
                    ),

        # Create a simple record hierarchy to represent the system
        self.records = RecordManager()

        nsr = make_nsr('test-nsr')

        vnfr_1 = make_vnfr('test-vnfr-1')
        vnfr_2 = make_vnfr('test-vnfr-1')

        vdur_1 = make_vdur(vim_id='test-vdur-1')
        vdur_1.vm_flavor.vcpu_count = 4
        vdur_1.vm_flavor.memory_mb = 16e3
        vdur_1.vm_flavor.storage_gb = 1e3

        vdur_2 = make_vdur(vim_id='test-vdur-2')
        vdur_2.vm_flavor.vcpu_count = 4
        vdur_2.vm_flavor.memory_mb = 16e3
        vdur_2.vm_flavor.storage_gb = 1e3

        vdur_3 = make_vdur(vim_id='test-vdur-3')
        vdur_3.vm_flavor.vcpu_count = 8
        vdur_3.vm_flavor.memory_mb = 32e3
        vdur_3.vm_flavor.storage_gb = 1e3

        nsr.constituent_vnfr_ref.append(vnfr_1.id)
        nsr.constituent_vnfr_ref.append(vnfr_2.id)

        vnfr_1.vdur.append(vdur_1)
        vnfr_1.vdur.append(vdur_2)
        vnfr_2.vdur.append(vdur_3)

        self.records.add_nsr(nsr)
        self.records.add_vnfr(vnfr_1)
        self.records.add_vnfr(vnfr_2)

        # Populate the NFVI monitor with static data
        vdu_metrics_1 = RwVnfrYang.YangData_Vnfr_VnfrCatalog_Vnfr_Vdur_NfviMetrics()
        vdu_metrics_1.vcpu.utilization = 10.0
        vdu_metrics_1.memory.used = 2e9
        vdu_metrics_1.storage.used = 1e10
        vdu_metrics_1.network.incoming.bytes = 1e5
        vdu_metrics_1.network.incoming.packets = 1e3
        vdu_metrics_1.network.incoming.byte_rate = 1e6
        vdu_metrics_1.network.incoming.packet_rate = 1e4
        vdu_metrics_1.network.outgoing.bytes = 1e5
        vdu_metrics_1.network.outgoing.packets = 1e3
        vdu_metrics_1.network.outgoing.byte_rate = 1e6
        vdu_metrics_1.network.outgoing.packet_rate = 1e4

        vdu_metrics_2 = RwVnfrYang.YangData_Vnfr_VnfrCatalog_Vnfr_Vdur_NfviMetrics()
        vdu_metrics_2.vcpu.utilization = 10.0
        vdu_metrics_2.memory.used = 2e9
        vdu_metrics_2.storage.used = 1e10
        vdu_metrics_2.network.incoming.bytes = 1e5
        vdu_metrics_2.network.incoming.packets = 1e3
        vdu_metrics_2.network.incoming.byte_rate = 1e6
        vdu_metrics_2.network.incoming.packet_rate = 1e4
        vdu_metrics_2.network.outgoing.bytes = 1e5
        vdu_metrics_2.network.outgoing.packets = 1e3
        vdu_metrics_2.network.outgoing.byte_rate = 1e6
        vdu_metrics_2.network.outgoing.packet_rate = 1e4

        vdu_metrics_3 = RwVnfrYang.YangData_Vnfr_VnfrCatalog_Vnfr_Vdur_NfviMetrics()
        vdu_metrics_3.vcpu.utilization = 20.0
        vdu_metrics_3.memory.used = 28e9
        vdu_metrics_3.storage.used = 1e10
        vdu_metrics_3.network.incoming.bytes = 1e5
        vdu_metrics_3.network.incoming.packets = 1e3
        vdu_metrics_3.network.incoming.byte_rate = 1e6
        vdu_metrics_3.network.incoming.packet_rate = 1e4
        vdu_metrics_3.network.outgoing.bytes = 1e5
        vdu_metrics_3.network.outgoing.packets = 1e3
        vdu_metrics_3.network.outgoing.byte_rate = 1e6
        vdu_metrics_3.network.outgoing.packet_rate = 1e4

        metrics = self.nfvi_monitor.metrics
        metrics[(self.cloud_account, vdur_1.vim_id)] = vdu_metrics_1
        metrics[(self.cloud_account, vdur_2.vim_id)] = vdu_metrics_2
        metrics[(self.cloud_account, vdur_3.vim_id)] = vdu_metrics_3

    def test_aggregation(self):
        """
        The hierarchy of the network service tested here is,

            test-nsr
            |-- test-vnfr-1
            |   |-- test-vdur-1
            |   \-- test-vdur-2
            \-- test-vnfr-2
                \-- test-vdur-3

        """
        loop = asyncio.get_event_loop()

        tasklet = MockTasklet(
                dts=None,
                loop=loop,
                log=logging.getLogger(),
                records=self.records,
                )

        # Create an instance of the NfviMetricsAggregator using a mock cloud
        # account and NFVI monitor
        aggregator = NfviMetricsAggregator(
                tasklet=tasklet,
                cloud_account=self.cloud_account,
                nfvi_monitor=self.nfvi_monitor,
                )

        # Run the event loop to retrieve the metrics from the aggregator
        task = loop.create_task(aggregator.request_ns_metrics('test-nsr'))
        loop.run_until_complete(task)

        ns_metrics = task.result()

        # Validate the metrics returned by the aggregator
        self.assertEqual(ns_metrics.vm.active_vm, 3)
        self.assertEqual(ns_metrics.vm.inactive_vm, 0)

        self.assertEqual(ns_metrics.vcpu.total, 16)
        self.assertEqual(ns_metrics.vcpu.utilization, 15.0)

        self.assertEqual(ns_metrics.memory.used, 32e9)
        self.assertEqual(ns_metrics.memory.total, 64e9)
        self.assertEqual(ns_metrics.memory.utilization, 50.0)

        self.assertEqual(ns_metrics.storage.used, 30e9)
        self.assertEqual(ns_metrics.storage.total, 3e12)
        self.assertEqual(ns_metrics.storage.utilization, 1.0)

        self.assertEqual(ns_metrics.network.incoming.bytes, 3e5)
        self.assertEqual(ns_metrics.network.incoming.packets, 3e3)
        self.assertEqual(ns_metrics.network.incoming.byte_rate, 3e6)
        self.assertEqual(ns_metrics.network.incoming.packet_rate, 3e4)

        self.assertEqual(ns_metrics.network.outgoing.bytes, 3e5)
        self.assertEqual(ns_metrics.network.outgoing.packets, 3e3)
        self.assertEqual(ns_metrics.network.outgoing.byte_rate, 3e6)
        self.assertEqual(ns_metrics.network.outgoing.packet_rate, 3e4)

    def test_publish_nfvi_metrics(self):
        loop = asyncio.get_event_loop()

        class RegistrationHandle(object):
            """
            Normally the aggregator uses the DTS RegistrationHandle to publish
            the NFVI metrics. This placeholder class is used to record the
            first NFVI metric data published by the aggregator, and then
            removes the NSR so that the aggregator terminates.

            """

            def __init__(self, test):
                self.xpath = None
                self.data = None
                self.test = test

            def deregister(self):
                pass

            def create_element(self, xpath, data):
                pass

            def update_element(self, xpath, data):
                # Record the results
                self.xpath = xpath
                self.data = data

                # Removing the NSR from the record manager will cause the
                # coroutine responsible for publishing the NFVI metric data to
                # terminate
                self.test.records.remove_nsr('test-nsr')

            @asyncio.coroutine
            def delete_element(self, xpath):
                assert xpath == self.xpath

        class Dts(object):
            """
            Placeholder Dts class that is used solely for the purpose of
            returning a RegistrationHandle to the aggregator.

            """
            def __init__(self, test):
                self.handle = RegistrationHandle(test)

            @asyncio.coroutine
            def register(self, *args, **kwargs):
                return self.handle

        dts = Dts(self)

        tasklet = MockTasklet(
                dts=dts,
                loop=loop,
                log=logging.getLogger(),
                records=self.records,
                )

        # Create an instance of the NfviMetricsAggregator using a mock cloud
        # account and NFVI monitor
        aggregator = NfviMetricsAggregator(
                tasklet=tasklet,
                cloud_account=self.cloud_account,
                nfvi_monitor=self.nfvi_monitor,
                )

        # Create a coroutine wrapper to timeout the test if it takes too long.
        @asyncio.coroutine
        def timeout_wrapper():
            coro = aggregator.publish_nfvi_metrics('test-nsr')
            yield from asyncio.wait_for(coro, timeout=1)

        loop.run_until_complete(timeout_wrapper())

        # Verify the data published by the aggregator
        self.assertEqual(dts.handle.data.vm.active_vm, 3)
        self.assertEqual(dts.handle.data.vm.inactive_vm, 0)

        self.assertEqual(dts.handle.data.vcpu.total, 16)
        self.assertEqual(dts.handle.data.vcpu.utilization, 15.0)

        self.assertEqual(dts.handle.data.memory.used, 32e9)
        self.assertEqual(dts.handle.data.memory.total, 64e9)
        self.assertEqual(dts.handle.data.memory.utilization, 50.0)

        self.assertEqual(dts.handle.data.storage.used, 30e9)
        self.assertEqual(dts.handle.data.storage.total, 3e12)
        self.assertEqual(dts.handle.data.storage.utilization, 1.0)

        self.assertEqual(dts.handle.data.network.incoming.bytes, 3e5)
        self.assertEqual(dts.handle.data.network.incoming.packets, 3e3)
        self.assertEqual(dts.handle.data.network.incoming.byte_rate, 3e6)
        self.assertEqual(dts.handle.data.network.incoming.packet_rate, 3e4)

        self.assertEqual(dts.handle.data.network.outgoing.bytes, 3e5)
        self.assertEqual(dts.handle.data.network.outgoing.packets, 3e3)
        self.assertEqual(dts.handle.data.network.outgoing.byte_rate, 3e6)
        self.assertEqual(dts.handle.data.network.outgoing.packet_rate, 3e4)


class TestRecordManager(unittest.TestCase):
    def setUp(self):
        pass

    def test_add_and_remove_nsr(self):
        records = RecordManager()

        # Create an empty NSR and add it to the record manager
        nsr = make_nsr()
        records.add_nsr(nsr)

        # The record manager should ignore this NSR because it contains no
        # VNFRs
        self.assertFalse(records.has_nsr(nsr.ns_instance_config_ref))


        # Now add a VNFR (with a VDUR) to the NSR and, once again, add it to
        # the record manager
        vdur = make_vdur()
        vnfr = make_vnfr()

        vnfr.vdur.append(vdur)

        nsr.constituent_vnfr_ref.append(vnfr.id)
        records.add_nsr(nsr)

        # The mapping from the NSR to the VNFR has been added, but the
        # relationship between the VNFR and the VDUR is not added.
        self.assertTrue(records.has_nsr(nsr.ns_instance_config_ref))
        self.assertFalse(records.has_vnfr(vnfr.id))


        # Try adding the same NSR again. The record manager should be
        # unchanged.
        records.add_nsr(nsr)

        self.assertEqual(1, len(records._nsr_to_vnfrs.keys()))
        self.assertEqual(1, len(records._nsr_to_vnfrs.values()))


        # Now remove the NSR and check that the internal structures have been
        # properly cleaned up.
        records.remove_nsr(nsr.ns_instance_config_ref)

        self.assertFalse(records.has_nsr(nsr.ns_instance_config_ref))
        self.assertFalse(records.has_vnfr(vnfr.id))

    def test_add_and_remove_vnfr(self):
        records = RecordManager()

        # Create an empty VNFR and add it to the record manager
        vnfr = make_vnfr()
        records.add_vnfr(vnfr)

        # The record manager should ignore this VNFR because it contains no
        # VDURs
        self.assertFalse(records.has_vnfr(vnfr.id))


        # Now add a VDUR to the VNFR and, once again, add it to the record
        # manager.
        vdur = make_vdur()
        vnfr.vdur.append(vdur)

        records.add_vnfr(vnfr)

        # The mapping from the VNFR to the VDUR has been added, and the VDUR
        # has been added the internal dictionary for mapping a vim_id to a
        # VDUR.
        self.assertTrue(records.has_vnfr(vnfr.id))
        self.assertIn(vdur.vim_id, records._vdurs)


        # Try adding the same VNFR again. The record manager should be
        # unchanged.
        records.add_vnfr(vnfr)

        self.assertEqual(1, len(records._vnfr_to_vdurs.keys()))
        self.assertEqual(1, len(records._vnfr_to_vdurs.values()))
        self.assertEqual(1, len(records._vdurs))


        # Now remove the VNFR and check that the internal structures have been
        # properly cleaned up.
        records.remove_vnfr(vnfr.id)

        self.assertFalse(records.has_vnfr(vnfr.id))
        self.assertNotIn(vdur.vim_id, records._vdurs)


def main(argv=sys.argv[1:]):
    logging.basicConfig(format='TEST %(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--verbose', action='store_true')

    args = parser.parse_args(argv)

    # Set the global logging level
    logging.getLogger().setLevel(logging.DEBUG if args.verbose else logging.ERROR)

    # The unittest framework requires a program name, so use the name of this
    # file instead (we do not want to have to pass a fake program name to main
    # when this is called from the interpreter).
    unittest.main(argv=[__file__] + argv,
            testRunner=xmlrunner.XMLTestRunner(
                output=os.environ["RIFT_MODULE_TEST"]))

if __name__ == '__main__':
    main()
