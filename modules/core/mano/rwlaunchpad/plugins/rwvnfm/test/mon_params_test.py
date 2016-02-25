#!/usr/bin/env python3

# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#


import asyncio
import base64
import logging
import os
import sys
import tornado.escape
import tornado.platform.asyncio
import tornado.testing
import tornado.web
import unittest
import xmlrunner

import rift.tasklets.rwvnfmtasklet.mon_params as mon_params


from gi.repository import VnfrYang

logging.basicConfig(format='TEST %(message)s', level=logging.DEBUG)
logger = logging.getLogger("mon_params_test.py")


class AsyncioTornadoTest(tornado.testing.AsyncHTTPTestCase):
    def setUp(self):
        self._loop = asyncio.get_event_loop()
        super().setUp()

    def get_new_ioloop(self):
        return tornado.platform.asyncio.AsyncIOMainLoop()


class MonParamsPingStatsTest(AsyncioTornadoTest):
    ping_path = r"/api/v1/ping/stats"
    ping_response = {
            'ping-request-tx-count': 5,
            'ping-response-rx-count': 10
            }

    mon_param_msg = VnfrYang.YangData_Vnfr_VnfrCatalog_Vnfr_MonitoringParam()
    mon_param_msg.from_dict({
            'id': '1',
            'name': 'ping-request-tx-count',
            'json_query_method': "NAMEKEY",
            'http_endpoint_ref': ping_path,
            'value_type': "INT",
            'description': 'no of ping requests',
            'group_tag': 'Group1',
            'widget_type': 'COUNTER',
            'units': 'packets'
            })

    endpoint_msg = VnfrYang.YangData_Vnfr_VnfrCatalog_Vnfr_HttpEndpoint()
    endpoint_msg.from_dict({
        'path': ping_path,
        'polling_interval_secs': 1,
        'username': 'admin',
        'password': 'password',
        'headers': [{'key': 'TEST_KEY', 'value': 'TEST_VALUE'}],
        })

    def create_endpoint(self, endpoint_msg):
        self.mon_port = self.get_http_port()
        endpoint = mon_params.HTTPEndpoint(
                logger,
                self._loop,
                "127.0.0.1",
                self.endpoint_msg,
                )
        # For each creation, update the descriptor as well
        endpoint_msg.port = self.mon_port

        return endpoint

    def create_mon_param(self):
        return mon_params.MonitoringParam(logger, self.mon_param_msg)

    def get_app(self):
        class PingStatsHandler(tornado.web.RequestHandler):
            def get(this):
                test_header = this.request.headers.get('TEST_KEY')
                if test_header is None or test_header != 'TEST_VALUE':
                    this.set_status(401)
                    this.finish()
                    return None

                auth_header = this.request.headers.get('Authorization')
                if auth_header is None or not auth_header.startswith('Basic '):
                    this.set_status(401)
                    this.set_header('WWW-Authenticate', 'Basic realm=Restricted')
                    this._transforms = []
                    this.finish()
                    return None

                auth_header = auth_header.encode('ascii')
                auth_decoded = base64.decodestring(auth_header[6:]).decode('ascii')
                login, password = auth_decoded.split(':', 2)
                login = login.encode('ascii')
                password = password.encode('ascii')
                is_auth = (login == b"admin" and password == b"password")

                if not is_auth:
                    this.set_status(401)
                    this.set_header('WWW-Authenticate', 'Basic realm=Restricted')
                    this._transforms = []
                    this.finish()
                    return None

                this.write(self.ping_response)

        return tornado.web.Application([
            (self.ping_path, PingStatsHandler),
            ])

    def test_value_convert(self):
        float_con = mon_params.ValueConverter("DECIMAL")
        int_con = mon_params.ValueConverter("INT")
        text_con = mon_params.ValueConverter("STRING")

        a = float_con.convert("1.23")
        self.assertEqual(a, 1.23)

        a = float_con.convert(1)
        self.assertEqual(a, float(1))

        t = text_con.convert(1.23)
        self.assertEqual(t, "1.23")

        t = text_con.convert("asdf")
        self.assertEqual(t, "asdf")

        i = int_con.convert(1.23)
        self.assertEqual(i, 1)

    def test_json_key_value_querier(self):
        kv_querier = mon_params.JsonKeyValueQuerier(logger, "ping-request-tx-count")
        value = kv_querier.query(tornado.escape.json_encode(self.ping_response))
        self.assertEqual(value, 5)

    def test_json_path_value_querier(self):
        kv_querier = mon_params.JsonPathValueQuerier(logger, '$.ping-request-tx-count')
        value = kv_querier.query(tornado.escape.json_encode(self.ping_response))
        self.assertEqual(value, 5)

    def test_object_path_value_querier(self):
        kv_querier = mon_params.ObjectPathValueQuerier(logger, "$.*['ping-request-tx-count']")
        value = kv_querier.query(tornado.escape.json_encode(self.ping_response))
        self.assertEqual(value, 5)

    def test_endpoint(self):
        @asyncio.coroutine
        def run_test():
            endpoint = self.create_endpoint(self.endpoint_msg)
            resp = yield from endpoint.poll()
            resp_json = tornado.escape.json_decode(resp)
            self.assertEqual(resp_json["ping-request-tx-count"], 5)
            self.assertEqual(resp_json["ping-response-rx-count"], 10)

        self._loop.run_until_complete(
                asyncio.wait_for(run_test(), 10, loop=self._loop)
                )

    def test_mon_param(self):
        a = self.create_mon_param()
        a.extract_value_from_response(tornado.escape.json_encode(self.ping_response))
        self.assertEqual(a.current_value, 5)
        self.assertEqual(a.msg.value_integer, 5)

    def test_endpoint_poller(self):
        endpoint = self.create_endpoint(self.endpoint_msg)
        mon_param = self.create_mon_param()
        poller = mon_params.EndpointMonParamsPoller(
                logger, self._loop, endpoint, [mon_param],
                )
        poller.start()

        self._loop.run_until_complete(asyncio.sleep(1, loop=self._loop))
        self.assertEqual(mon_param.current_value, 5)

        poller.stop()

    def test_params_controller(self):
        new_port = self.get_http_port()
        # Update port after new port is initialized
        self.endpoint_msg.port = new_port
        ctrl = mon_params.VnfMonitoringParamsController(
                logger, self._loop, "1", "127.0.0.1", 
                [self.endpoint_msg], [self.mon_param_msg],
                )
        ctrl.start()

        self._loop.run_until_complete(asyncio.sleep(1, loop=self._loop))

        ctrl.stop()

        self.assertEqual(1, len(ctrl.mon_params))
        mon_param = ctrl.mon_params[0]
        self.assertEqual(mon_param.current_value, 5)


class AsyncioTornadoHttpsTest(tornado.testing.AsyncHTTPSTestCase):
    def setUp(self):
        self._loop = asyncio.get_event_loop()
        super().setUp()

    def get_new_ioloop(self):
        return tornado.platform.asyncio.AsyncIOMainLoop()


class MonParamsPingStatsHttpsTest(AsyncioTornadoHttpsTest):
    ping_path = r"/api/v1/ping/stats"
    ping_response = {
            'ping-request-tx-count': 5,
            'ping-response-rx-count': 10
            }

    mon_param_msg = VnfrYang.YangData_Vnfr_VnfrCatalog_Vnfr_MonitoringParam()
    mon_param_msg.from_dict({
            'id': '1',
            'name': 'ping-request-tx-count',
            'json_query_method': "NAMEKEY",
            'http_endpoint_ref': ping_path,
            'value_type': "INT",
            'description': 'no of ping requests',
            'group_tag': 'Group1',
            'widget_type': 'COUNTER',
            'units': 'packets'
            })

    endpoint_msg = VnfrYang.YangData_Vnfr_VnfrCatalog_Vnfr_HttpEndpoint()
    endpoint_msg.from_dict({
        'path': ping_path,
        'https': 'true',
        'polling_interval_secs': 1,
        'username': 'admin',
        'password': 'password',
        'headers': [{'key': 'TEST_KEY', 'value': 'TEST_VALUE'}],
        })

    def create_endpoint(self, endpoint_msg):
        self.mon_port = self.get_http_port()
        endpoint = mon_params.HTTPEndpoint(
                logger,
                self._loop,
                "127.0.0.1",
                self.endpoint_msg,
                )
        # For each creation, update the descriptor as well
        endpoint_msg.port = self.mon_port

        return endpoint

    def create_mon_param(self):
        return mon_params.MonitoringParam(logger, self.mon_param_msg)

    def get_app(self):
        class PingStatsHandler(tornado.web.RequestHandler):
            def get(this):
                test_header = this.request.headers.get('TEST_KEY')
                if test_header is None or test_header != 'TEST_VALUE':
                    this.set_status(401)
                    this.finish()
                    return None

                auth_header = this.request.headers.get('Authorization')
                if auth_header is None or not auth_header.startswith('Basic '):
                    this.set_status(401)
                    this.set_header('WWW-Authenticate', 'Basic realm=Restricted')
                    this._transforms = []
                    this.finish()
                    return None

                auth_header = auth_header.encode('ascii')
                auth_decoded = base64.decodestring(auth_header[6:]).decode('ascii')
                login, password = auth_decoded.split(':', 2)
                login = login.encode('ascii')
                password = password.encode('ascii')
                is_auth = (login == b"admin" and password == b"password")

                if not is_auth:
                    this.set_status(401)
                    this.set_header('WWW-Authenticate', 'Basic realm=Restricted')
                    this._transforms = []
                    this.finish()
                    return None

                this.write(self.ping_response)

        return tornado.web.Application([
            (self.ping_path, PingStatsHandler),
            ])

    def test_value_convert(self):
        float_con = mon_params.ValueConverter("DECIMAL")
        int_con = mon_params.ValueConverter("INT")
        text_con = mon_params.ValueConverter("STRING")

        a = float_con.convert("1.23")
        self.assertEqual(a, 1.23)

        a = float_con.convert(1)
        self.assertEqual(a, float(1))

        t = text_con.convert(1.23)
        self.assertEqual(t, "1.23")

        t = text_con.convert("asdf")
        self.assertEqual(t, "asdf")

        i = int_con.convert(1.23)
        self.assertEqual(i, 1)

    def test_json_key_value_querier(self):
        kv_querier = mon_params.JsonKeyValueQuerier(logger, "ping-request-tx-count")
        value = kv_querier.query(tornado.escape.json_encode(self.ping_response))
        self.assertEqual(value, 5)

    def test_endpoint(self):
        @asyncio.coroutine
        def run_test():
            endpoint = self.create_endpoint(self.endpoint_msg)
            resp = yield from endpoint.poll()
            resp_json = tornado.escape.json_decode(resp)
            self.assertEqual(resp_json["ping-request-tx-count"], 5)
            self.assertEqual(resp_json["ping-response-rx-count"], 10)

        self._loop.run_until_complete(
                asyncio.wait_for(run_test(), 10, loop=self._loop)
                )

    def test_mon_param(self):
        a = self.create_mon_param()
        a.extract_value_from_response(tornado.escape.json_encode(self.ping_response))
        self.assertEqual(a.current_value, 5)
        self.assertEqual(a.msg.value_integer, 5)

    def test_endpoint_poller(self):
        endpoint = self.create_endpoint(self.endpoint_msg)
        mon_param = self.create_mon_param()
        poller = mon_params.EndpointMonParamsPoller(
                logger, self._loop, endpoint, [mon_param],
                )
        poller.start()

        self._loop.run_until_complete(asyncio.sleep(1, loop=self._loop))
        self.assertEqual(mon_param.current_value, 5)

        poller.stop()

    def test_params_controller(self):
        new_port = self.get_http_port()
        # Update port after new port is initialized
        self.endpoint_msg.port = new_port
        ctrl = mon_params.VnfMonitoringParamsController(
                logger, self._loop, "1", "127.0.0.1", 
                [self.endpoint_msg], [self.mon_param_msg],
                )
        ctrl.start()

        self._loop.run_until_complete(asyncio.sleep(1, loop=self._loop))

        ctrl.stop()

        self.assertEqual(1, len(ctrl.mon_params))
        mon_param = ctrl.mon_params[0]
        self.assertEqual(mon_param.current_value, 5)


class VRouterStatsTest(unittest.TestCase):
    system_response = {
        "system": {
            "cpu": [
                {
                    "usage": 2.35,
                    "cpu": "all"
                },
                {
                    "usage": 5.35,
                    "cpu": "1"
                }
            ]
        }
    }

    def test_object_path_value_querier(self):
        kv_querier = mon_params.ObjectPathValueQuerier(logger, "$.system.cpu[@.cpu is 'all'].usage")
        value = kv_querier.query(tornado.escape.json_encode(self.system_response))
        self.assertEqual(value, 2.35)


class TrafsinkStatsTest(unittest.TestCase):
    system_response = {
       "rw-vnf-base-opdata:port-state": [
         {
           "ip": [
             {
               "address": "12.0.0.3/24"
             }
           ],
           "rw-trafgen-data:trafgen-info": {
             "src_l4_port": 1234,
             "dst_l4_port": 5678,
             "dst_ip_address": "192.168.1.1",
             "tx_state": "Off",
             "dst_mac_address": "00:00:00:00:00:00",
             "tx_mode": "single-template",
             "packet-count": 0,
             "tx-cycles": 5478,
             "tx_burst": 16,
             "src_ip_address": "192.168.0.1",
             "pkt_size": 64,
             "src_mac_address": "fa:16:3e:07:b1:52",
             "descr-string": "",
             "tx_rate": 100
           },
           "counters": {
             "input-errors": 0,
             "output-bytes": 748,
             "input-pause-xoff-pkts": 0,
             "input-badcrc-pkts": 0,
             "input-bytes": 62,
             "rx-rate-mbps": 9576,
             "output-pause-xoff-pkts": 0,
             "input-missed-pkts": 0,
             "input-packets": 1,
             "output-errors": 0,
             "tx-rate-mbps": 0,
             "input-pause-xon-pkts": 0,
             "output-pause-xon-pkts": 0,
             "tx-rate-pps": 0,
             "input-mcast-pkts": 0,
             "rx-rate-pps": 0,
             "output-packets": 6,
             "input-nombuf-pkts": 0
           },
           "info": {
             "numa-socket": 0,
             "transmit-queues": 1,
             "privatename": "eth_uio:pci=0000:00:04.0",
             "duplex": "full-duplex",
             "virtual-fabric": "No",
             "link-state": "up",
             "rte-port-id": 0,
             "fastpath-instance": 1,
             "id": 0,
             "app-name": "rw_trafgen",
             "speed": 10000,
             "receive-queues": 1,
             "descr-string": "",
             "mac": "fa:16:3e:07:b1:52"
           },
           "portname": "trafsink_vnfd/cp0",
           "queues": {
             "rx-queue": [
               {
                 "packets": 1,
                 "bytes-MB": 0,
                 "qid": 0,
                 "rate-mbps": 0,
                 "rate-pps": 0
               }
             ],
             "tx-queue": [
               {
                 "bytes-MB": 0,
                 "packets": 6,
                 "rate-pps": 0,
                 "errors": 0,
                 "qid": 0,
                 "rate-mbps": 0
               }
             ]
           }
         }
       ]
     }

    def test_object_path_value_querier(self):
        kv_querier = mon_params.ObjectPathValueQuerier(logger, "$..*[@.portname is 'trafsink_vnfd/cp0'].counters.'rx-rate-mbps'")
        value = kv_querier.query(tornado.escape.json_encode(self.system_response))
        self.assertEqual(value, 9576)


def main(argv=sys.argv[1:]):

    # The unittest framework requires a program name, so use the name of this
    # file instead (we do not want to have to pass a fake program name to main
    # when this is called from the interpreter).
    unittest.main(
            argv=[__file__] + argv,
            testRunner=xmlrunner.XMLTestRunner(output=os.environ["RIFT_MODULE_TEST"])
            )

if __name__ == '__main__':
    main()

