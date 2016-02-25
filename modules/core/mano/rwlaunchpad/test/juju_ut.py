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
        RwConfigAgentYang,
        NsrYang
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
    def configure_cloud_account(self, cloud_name="cloud1"):
        account = rwcloudyang.CloudAccount()
        account.name          = cloud_name
        account.account_type  = "mock"
        account.mock.username = "mock_user"

        account_xpath = "C,/rw-cloud:cloud/rw-cloud:account[rw-cloud:name='{}']".format(cloud_name)
        self.log.info("Configuring cloud-account: %s", account)
        yield from self.dts.query_create(account_xpath,
                                    rwdts.Flag.ADVISE | rwdts.Flag.TRACE,
                                    account)

    @asyncio.coroutine
    def configure_config_agent(self):
        account_xpath = "C,/rw-config-agent:config-agent/account[name='Juju1 config']"

        juju1 = RwConfigAgentYang.ConfigAgentAccount.from_dict({
                "name": "Juju1 config",
                "account_type": "juju",
                "juju": {
                    "ip_address": "1.1.1.1",
                    "port": 9000,
                    "user": "foo",
                    "secret": "1232"
                }
            })

        cfg_agt = RwConfigAgentYang.ConfigAgent()
        cfg_agt.account.append(juju1)
        cfg_agt.as_dict()

        yield from self.dts.query_create(
                account_xpath,
                rwdts.Flag.ADVISE,
                juju1,
                )


    @asyncio.coroutine
    def configure_config_primitive(self, nsr_id):
        job_data = NsrYang.YangInput_Nsr_ExecNsConfigPrimitive.from_dict({
            "name": "Add Corporation",
            "nsr_id_ref": nsr_id,
            "vnf_list": [{
                "vnfr_id_ref": "10631555-757e-4924-96e6-41a0297a9406",
                "member_vnf_index_ref": 1,
                "vnf_primitive": [{
                    "name": "create-update-user",
                    "parameter": [
                           {"name" : "number", "value": "1234334"},
                           {"name" : "password", "value": "1234334"},
                        ]
                    }]
                }]
            
        })
        yield from self.dts.query_rpc(
                "/nsr:exec-ns-config-primitive",
                0,
                job_data,
                )

    @rift.test.dts.async_test
    def test_ping_pong_nsm_instantiate(self):
        yield from self.wait_tasklets()
        yield from self.configure_cloud_account("mock_account")
        yield from self.configure_config_agent()
        yield from self.publish_desciptors(num_internal_vlrs=0)

        nsr_id = yield from self.ping_pong.publish_nsr_config("mock_account")
        yield from asyncio.sleep(10, loop=self.loop)

        res_iter = yield from self.dts.query_read("D,/nsr:ns-instance-opdata/nsr:nsr")
        for i in res_iter:
            result = yield from i

        print ("**", result)
        # yield from self.configure_config_primitive(nsr_id)
        yield from asyncio.sleep(10, loop=self.loop)

        # nsrs = yield from self.querier.get_nsr_opdatas()
        # nsr = nsrs[0]


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

# vim: sw
