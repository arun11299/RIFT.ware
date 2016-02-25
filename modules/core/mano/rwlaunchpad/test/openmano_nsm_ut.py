#!/usr/bin/env python3

# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import argparse
import asyncio
import logging
import os
import sys
import time
import unittest
import uuid

import xmlrunner

from gi.repository import (
        RwDts as rwdts,
        RwLaunchpadYang as launchpadyang,
        RwNsmYang as rwnsmyang,
        RwCloudYang as rwcloudyang,
        RwResourceMgrYang,
        )
import rift.tasklets
import rift.test.dts

import mano_ut


if sys.version_info < (3, 4, 4):
    asyncio.ensure_future = asyncio.async


class OpenManoNsmTestCase(mano_ut.ManoTestCase):
    """
    DTS GI interface unittests

    Note:  Each tests uses a list of asyncio.Events for staging through the
    test.  These are required here because we are bring up each coroutine
    ("tasklet") at the same time and are not implementing any re-try
    mechanisms.  For instance, this is used in numerous tests to make sure that
    a publisher is up and ready before the subscriber sends queries.  Such
    event lists should not be used in production software.
    """

    @classmethod
    def configure_suite(cls, rwmain):
        launchpad_build_dir = os.path.join(
                cls.top_dir,
                '.build/modules/core/mc/core_mc-build/rwlaunchpad'
                )

        rwmain.add_tasklet(
                os.path.join(launchpad_build_dir, 'plugins/rwnsm'),
                'rwnsmtasklet'
                )

        cls.waited_for_tasklets = False

    @classmethod
    def configure_schema(cls):
        return rwnsmyang.get_schema()

    @classmethod
    def configure_timeout(cls):
        return 240

    @asyncio.coroutine
    def wait_tasklets(self):
        if not OpenManoNsmTestCase.waited_for_tasklets:
            OpenManoNsmTestCase.waited_for_tasklets = True
            self._wait_event = asyncio.Event(loop=self.loop)
            yield from asyncio.sleep(5, loop=self.loop)
            self._wait_event.set()

        yield from self._wait_event.wait()

    @asyncio.coroutine
    def publish_desciptors(self, num_external_vlrs=1, num_internal_vlrs=1, num_ping_vms=1):
        yield from self.ping_pong.publish_desciptors(
                num_external_vlrs,
                num_internal_vlrs,
                num_ping_vms
                )

    def unpublish_descriptors(self):
        self.ping_pong.unpublish_descriptors()

    @asyncio.coroutine
    def wait_until_nsr_active_or_failed(self, nsr_id, timeout_secs=20):
        start_time = time.time()
        while (time.time() - start_time) < timeout_secs:
            nsrs = yield from self.querier.get_nsr_opdatas(nsr_id)
            if len(nsrs) == 0:
                continue
            self.assertEqual(1, len(nsrs))
            if nsrs[0].operational_status in ['running', 'failed']:
                return

            self.log.debug("Rcvd NSR with %s status", nsrs[0].operational_status)
            yield from asyncio.sleep(2, loop=self.loop)

        self.assertIn(nsrs[0].operational_status, ['running', 'failed'])

    def configure_test(self, loop, test_id):
        self.log.debug("STARTING - %s", self.id())
        self.tinfo = self.new_tinfo(self.id())
        self.dts = rift.tasklets.DTS(self.tinfo, self.schema, self.loop)
        self.ping_pong = mano_ut.PingPongDescriptorPublisher(self.log, self.loop, self.dts)
        self.querier = mano_ut.ManoQuerier(self.log, self.dts)

        # Add a task to wait for tasklets to come up
        asyncio.ensure_future(self.wait_tasklets(), loop=self.loop)

    @asyncio.coroutine
    def configure_cloud_account(self):
        account_xpath = "C,/rw-cloud:cloud-account"
        account = rwcloudyang.CloudAccount()
        account.name = "openmano_name"
        account.account_type = "openmano"
        account.openmano.host = "10.64.5.73"
        account.openmano.port = 9090
        account.openmano.tenant_id = "eecfd632-bef1-11e5-b5b8-0800273ab84b"
        self.log.info("Configuring cloud-account: %s", account)
        yield from self.dts.query_create(
                account_xpath,
                rwdts.Flag.ADVISE,
                account,
                )

    @rift.test.dts.async_test
    def test_ping_pong_nsm_instantiate(self):
        yield from self.wait_tasklets()
        yield from self.configure_cloud_account()
        yield from self.publish_desciptors(num_internal_vlrs=0)

        nsr_id = yield from self.ping_pong.publish_nsr_config()

        yield from self.wait_until_nsr_active_or_failed(nsr_id)

        yield from self.verify_nsd_ref_count(self.ping_pong.nsd_id, 1)
        yield from self.verify_nsr_state(nsr_id, "running")
        yield from self.verify_num_vlrs(0)

        yield from self.verify_num_nsr_vnfrs(nsr_id, 2)

        yield from self.verify_num_vnfrs(2)
        nsr_vnfs = yield from self.get_nsr_vnfs(nsr_id)
        yield from self.verify_vnf_state(nsr_vnfs[0], "running")
        yield from self.verify_vnf_state(nsr_vnfs[1], "running")

        yield from self.terminate_nsr(nsr_id)
        yield from asyncio.sleep(2, loop=self.loop)

        yield from self.verify_nsr_deleted(nsr_id)
        yield from self.verify_nsd_ref_count(self.ping_pong.nsd_id, 0)
        yield from self.verify_num_vnfrs(0)

def main():
    runner = xmlrunner.XMLTestRunner(output=os.environ["RIFT_MODULE_TEST"])

    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--verbose', action='store_true')
    parser.add_argument('-n', '--no-runner', action='store_true')
    args, unittest_args = parser.parse_known_args()
    if args.no_runner:
        runner = None

    OpenManoNsmTestCase.log_level = logging.DEBUG if args.verbose else logging.WARN

    unittest.main(testRunner=runner, argv=[sys.argv[0]]+unittest_args)

if __name__ == '__main__':
    main()
