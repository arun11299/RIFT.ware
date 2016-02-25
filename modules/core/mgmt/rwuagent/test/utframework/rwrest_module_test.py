#!/usr/bin/env python3
"""
#
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
#
# @file rwrest_module_test.py
# @author Max Beckett
# @date 12/8/2015
#
"""
import json
import logging
import os
from subprocess import Popen, DEVNULL
import sys
import time
import unittest

import tornado.httpclient
import tornado.web
import xmlrunner

logger = logging.getLogger(__name__)

WAIT_TIME = 45 #seconds

def _collapse_string(string):
    return ''.join([line.strip() for line in string.splitlines()])

def _ordered(obj):
    '''sort json dictionaries for comparison'''
    if isinstance(obj, dict):
        for k, v in obj.items():
            if not isinstance(v, list) and not isinstance(v, dict):
                obj[k] = str(v)
        return sorted((k, _ordered(v)) for k, v in obj.items())
    if isinstance(obj, list):
        return sorted(_ordered(x) for x in obj)
    else:
        return obj

class TestRiftRest(unittest.TestCase):

    sys_proc=None

    @classmethod
    def setUpClass(cls):
       # Start the mgmt-agent system test in background
       cls.log_file = open("logfile", "w")

       install_dir = os.environ.get("RIFT_INSTALL")
       demo_dir = os.path.join(install_dir, 'demos')
       testbed = os.path.join(demo_dir, 'mgmt_tbed.py')

       try:
         cls.sys_proc=Popen([testbed], stdin=DEVNULL, stdout=cls.log_file, stderr=cls.log_file,
                             preexec_fn=os.setsid)
       except Exception as e:
         print("Failed to start system test.. error: {0}".format(e))
         raise

       print("Started the Mgmt Agent Mini System Test..sleeping for {} secs".format(WAIT_TIME))
       time.sleep(WAIT_TIME)

       cls.http_client = tornado.httpclient.HTTPClient()
       cls.http_headers = {
            "Accept" : "application/vnd.yang.data+json",
            "Content-Type" : "application/vnd.yang.data+json",
       }

    @classmethod
    def tearDownClass(cls):
        print("Stopping the Mgmt Agent Mini System Test..")
        cls.sys_proc.terminate()
        cls.log_file.close()

    def test_misc(self):
        self.maxDiff = None

        url = "http://localhost:8888/api/running/misc?deep"
        json_body = _collapse_string('''
{
    "bool-leaf":"True",
    "empty-leaf":[null],
    "enum-leaf":"a",
    "int-leaf":42,
    "list-a":[
        {
            "id":0,
            "foo":"asdf"
        }
    ],
    "list-b":[
        {
            "id":0
        }
    ],
    "numbers":[
        {
            "int8-leaf":0,
            "int16-leaf":0,
            "int32-leaf":0,
            "int64-leaf":0,
            "uint8-leaf":0,
            "uint16-leaf":0,
            "uint32-leaf":0,
            "uint64-leaf":0,
            "decimal-leaf":0.0
        },
        {
            "int8-leaf":"1",
            "int16-leaf":"0",
            "int32-leaf":"0",
            "int64-leaf":"0",
            "uint8-leaf":"0",
            "uint16-leaf":"0",
            "uint32-leaf":"0",
            "uint64-leaf":"0",
            "decimal-leaf":"0.0"
        }
    ]
}
        ''')

        expected_json = _collapse_string('''
{
  "example:misc":{
    "bool-leaf":"true",
    "empty-leaf":[null],
    "enum-leaf":"a",
    "int-leaf":42,
    "list-a":[
        {
            "id":0,
            "foo":"asdf"
        }
    ],
    "list-b":[
        {
            "id":0
        }
    ],
    "numbers":[
        {
            "int8-leaf":0,
            "int16-leaf":0,
            "int32-leaf":0,
            "int64-leaf":0,
            "uint8-leaf":0,
            "uint16-leaf":0,
            "uint32-leaf":0,
            "uint64-leaf":0,
            "decimal-leaf":0.0
        },
        {
            "int8-leaf":"1",
            "int16-leaf":"0",
            "int32-leaf":"0",
            "int64-leaf":"0",
            "uint8-leaf":"0",
            "uint16-leaf":"0",
            "uint32-leaf":"0",
            "uint64-leaf":"0",
            "decimal-leaf":"0.0"
        }
    ]
  }
}
        ''')

        put_request = tornado.httpclient.HTTPRequest(
            url,
            headers=self.http_headers,
            method="PUT",
            body=json_body,
            auth_username="admin",
            auth_password="admin",
        )

        put_result = self.http_client.fetch(put_request)
        put_status= put_result.code

        get_request = tornado.httpclient.HTTPRequest(
            url,
            headers=self.http_headers,
            method="GET",
            auth_username="admin",
            auth_password="admin",
        )

        get_result = self.http_client.fetch(get_request)
        get_status= put_result.code

        expected = _ordered(json.loads(expected_json))
        actual = _ordered(json.loads(get_result.body.decode("utf-8")))

        self.assertEquals(actual, expected)


def main(argv=sys.argv[1:]):
    logging.basicConfig(format='TEST %(message)s')

    # The unittest framework requires a program name, so use the name of this
    # file instead (we do not want to have to pass a fake program name to main
    # when this is called from the interpreter).
    unittest.main(argv=[__file__] + argv,
            testRunner=xmlrunner.XMLTestRunner(
                output=os.environ["RIFT_MODULE_TEST"]))

if __name__ == '__main__':
    main()
