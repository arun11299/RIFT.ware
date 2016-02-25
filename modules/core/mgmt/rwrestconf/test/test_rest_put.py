#!/usr/bin/env python3

# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Max Beckett
# Creation Date: 7/10/2015
# 

import argparse
import json
import logging
import os
import sys
import unittest
import xmlrunner

from rift.restconf import (
    ConfdRestTranslator,
    convert_xml_to_collection,
    create_xpath_from_url,
    XmlToJsonTranslator,
    load_schema_root,
    load_multiple_schema_root,
)


def _collapse_string(string):
    return ''.join([line.strip() for line in string.splitlines()])

def _ordered(obj):
    '''sort json dictionaries for comparison'''
    if isinstance(obj, dict):
        return sorted((k, _ordered(v)) for k, v in obj.items())
    if isinstance(obj, list):
        return sorted(_ordered(x) for x in obj)
    else:
        return obj

class TestRestPut(unittest.TestCase):


    def test_json_body_0(self):
        self.maxDiff = None
        url = "/api/config/top-container-shallow"
        body = _collapse_string('''
{
  "a" : "some leaf 0"
}
        ''')

        expected_xml = _collapse_string('''
<config xmlns:xc="urn:ietf:params:xml:ns:netconf:base:1.0"><vehicle-a:top-container-shallow xmlns:vehicle-a="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><a xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">some leaf 0</a></vehicle-a:top-container-shallow></config>
        ''')

        schema = load_multiple_schema_root(["vehicle-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("PUT", url, (body,"json"))

        self.assertEquals(actual_xml, expected_xml)

    def test_json_body_1(self):
        self.maxDiff = None
        url = "/api/config/top-list-shallow/"
        body = _collapse_string('''
{
  "top-list-shallow"  :
  [
    {
      "k" : "some key 1"
    },
    {
      "k" : "some key 2"
    }

  ]
}

        ''')

        expected_xml = _collapse_string('''
<config xmlns:xc="urn:ietf:params:xml:ns:netconf:base:1.0"><top-list-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" xc:operation="replace"><k xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">some key 1</k></top-list-shallow><top-list-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" xc:operation="replace"><k xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">some key 2</k></top-list-shallow></config>
        ''')

        schema = load_multiple_schema_root(["vehicle-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("PUT", url, (body,"json"))

        self.assertEquals(actual_xml, expected_xml)


    def test_json_body_2(self):
        self.maxDiff = None
        url = "/api/config/multi-key"
        body = _collapse_string('''
{
  "multi-key" :
  [
    {
      "foo" : "key part 1",
      "bar" : "key part 2",
      "treasure" : "some string"
    },
    {
      "foo" : "other key part 1",
      "bar" : "other key part 2",
      "treasure" : "some other string"
    }
  ]
}
        ''')

        expected_xml = _collapse_string('''
<config xmlns:xc="urn:ietf:params:xml:ns:netconf:base:1.0"><multi-key xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" xc:operation="replace"><foo xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">key part 1</foo><bar xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">key part 2</bar><treasure xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">some string</treasure></multi-key><multi-key xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" xc:operation="replace"><foo xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">other key part 1</foo><bar xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">other key part 2</bar><treasure xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">some other string</treasure></multi-key></config>
        ''')

        schema = load_multiple_schema_root(["vehicle-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("PUT", url, (body,"json"))

        self.assertEquals(actual_xml, expected_xml)

    def test_json_body_3(self):
        self.maxDiff = None
        url = "/api/config/top-list-deep"
        body = _collapse_string('''
{
  "top-list-deep" :
  [
    {
      "k" : "some key",
      "inner-list" :
      [
        {
	  "k" : "some other key",
	  "a" : "some string",
	  "inner-container" : {
	    "a" : "some other string"
	  }
	}
      ],
      "inner-container-shallow" :
      {
        "a" : "yet a third string"
      },
      "inner-container-deep" :
      {
        "bottom-list-shallow" :
	[
	  {
	  "k" : "yet a third key"
	  }
	]
      }
    }
  ]
}        ''')

        expected_xml = _collapse_string('''
<config xmlns:xc="urn:ietf:params:xml:ns:netconf:base:1.0"><top-list-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" xc:operation="replace"><k xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">some key</k><inner-list xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" xc:operation="replace"><k xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">some other key</k><a xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">some string</a><inner-container xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><a xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">some other string</a></inner-container></inner-list><inner-container-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><a xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">yet a third string</a></inner-container-shallow><inner-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><bottom-list-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" xc:operation="replace"><k xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">yet a third key</k></bottom-list-shallow></inner-container-deep></top-list-deep></config>
        ''')

        schema = load_multiple_schema_root(["vehicle-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("PUT", url, (body,"json"))

        self.assertEquals(actual_xml, expected_xml)

    def test_json_body_4(self):
        self.maxDiff = None
        url = "/api/config/top-container-deep"
        body = _collapse_string('''
{
    "a":"some kind of string",
    "inner-list-shallow":[
        {
            "k":"some key thing"
        },
        {
            "k":"some other key thing"
        }
    ],
    "inner-container":{
        "a":"another string",
        "inner-list":[
            {
                "k":"another key thing"
            }
        ]
    },
    "inner-list-deep":[
        {
            "k":"inner key",
            "inner-container-shallow":{
                "a":"an inner string"
            },
            "inner-container-deep":{
                "bottom-list-shallow":[
                    {
                        "k":"bottom key"
                    }
                ]
            }
        }
    ]
}
        ''')

        expected_xml = _collapse_string('''
<config xmlns:xc="urn:ietf:params:xml:ns:netconf:base:1.0"><vehicle-a:top-container-deep xmlns:vehicle-a="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><a xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">some kind of string</a><inner-list-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" xc:operation="replace"><k xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">some key thing</k></inner-list-shallow><inner-list-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" xc:operation="replace"><k xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">some other key thing</k></inner-list-shallow><inner-container xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><a xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">another string</a><inner-list xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" xc:operation="replace"><k xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">another key thing</k></inner-list></inner-container><inner-list-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" xc:operation="replace"><k xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">inner key</k><inner-container-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><a xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">an inner string</a></inner-container-shallow><inner-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><bottom-list-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" xc:operation="replace"><k xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">bottom key</k></bottom-list-shallow></inner-container-deep></inner-list-deep></vehicle-a:top-container-deep></config>
        ''')

        schema = load_multiple_schema_root(["vehicle-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("PUT", url, (body,"json"))

        self.assertEquals(actual_xml, expected_xml)

    def test_json_body_5(self):
        self.maxDiff = None
        url = "/api/config/top-list-deep/key1/inner-list/key2"
        body = _collapse_string('''
{
    "inner-list":[
        {
            "k":"key2"
        }
    ]
}
        ''')

        expected_xml = _collapse_string('''
<config xmlns:xc="urn:ietf:params:xml:ns:netconf:base:1.0"><top-list-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k>key1</k><inner-list xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" xc:operation="replace"><k xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">key2</k></inner-list></top-list-deep></config>
        ''')

        schema = load_multiple_schema_root(["vehicle-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("PUT", url, (body,"json"))

        self.assertEquals(actual_xml, expected_xml)

    def test_json_body_6(self):
        self.maxDiff = None
        url = "/api/config/top-container-deep/inner-list-shallow"
        body = _collapse_string('''
{
    "inner-list-shallow":[
        {
            "k":"some key thing"
        },
        {
            "k":"some other key thing"
        }
    ]
}
        ''')

        expected_xml = _collapse_string('''
<config xmlns:xc="urn:ietf:params:xml:ns:netconf:base:1.0"><top-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><inner-list-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" xc:operation="replace"><k xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">some key thing</k></inner-list-shallow><inner-list-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" xc:operation="replace"><k xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">some other key thing</k></inner-list-shallow></top-container-deep></config>
        ''')

        schema = load_multiple_schema_root(["vehicle-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("PUT", url, (body,"json"))

        self.assertEquals(actual_xml, expected_xml)

    def test_json_body_7(self):
        self.maxDiff = None
        url = "/api/config/top-list-deep/key1/inner-list/key2"
        body = _collapse_string('''
{
    "inner-list":[
        {
            "k":"key2"
        }
    ]
}
        ''')

        expected_xml = _collapse_string('''
<config xmlns:xc="urn:ietf:params:xml:ns:netconf:base:1.0"><top-list-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k>key1</k><inner-list xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" xc:operation="replace"><k xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">key2</k></inner-list></top-list-deep></config>
        ''')

        schema = load_multiple_schema_root(["vehicle-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("PUT", url, (body,"json"))

        self.assertEquals(actual_xml, expected_xml)

    def test_json_body_8(self):
        self.maxDiff = None
        url = "/api/config/top-container-deep/inner-container"
        body = _collapse_string('''
{
    "a":"another string",
    "inner-list":[
        {
            "k":"another key thing"
        }
    ]
}
        ''')

        expected_xml = _collapse_string('''
<config xmlns:xc="urn:ietf:params:xml:ns:netconf:base:1.0"><top-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><vehicle-a:inner-container xmlns:vehicle-a="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><a xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">another string</a><inner-list xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" xc:operation="replace"><k xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">another key thing</k></inner-list></vehicle-a:inner-container></top-container-deep></config>
        ''')

        schema = load_multiple_schema_root(["vehicle-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("PUT", url, (body,"json"))

        self.assertEquals(actual_xml, expected_xml)

    def test_json_body_9(self):
        self.maxDiff = None
        url = "/api/config/top-list-deep/some%20key/inner-container-shallow"
        body = _collapse_string('''
{
    "a" : "yet a third string"
}
        ''')

        expected_xml = _collapse_string('''
<config xmlns:xc="urn:ietf:params:xml:ns:netconf:base:1.0"><top-list-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k>some%20key</k><vehicle-a:inner-container-shallow xmlns:vehicle-a="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><a xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">yet a third string</a></vehicle-a:inner-container-shallow></top-list-deep></config>
        ''')

        schema = load_multiple_schema_root(["vehicle-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("PUT", url, (body,"json"))

        self.assertEquals(actual_xml, expected_xml)

    def test_json_body_10(self):
        self.maxDiff = None
        url = "/api/config/misc"
        body = _collapse_string('''
{
        "empty-leaf" : [null]
}
        ''')

        expected_xml = _collapse_string('''
<config xmlns:xc="urn:ietf:params:xml:ns:netconf:base:1.0"><vehicle-a:misc xmlns:vehicle-a="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><empty-leaf xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"></empty-leaf></vehicle-a:misc></config>
        ''')

        schema = load_multiple_schema_root(["vehicle-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("PUT", url, (body,"json"))

        self.assertEquals(actual_xml, expected_xml)

    def test_json_body_11(self):
        self.maxDiff = None
        url = "/api/config/misc"
        body = _collapse_string('''
{
    "bool-leaf":true,
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
            "decimal-leaf":0
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
            "decimal-leaf":"0"
        }
    ]
}
        ''')

        expected_xml = _collapse_string('''
<config xmlns:xc="urn:ietf:params:xml:ns:netconf:base:1.0"><vehicle-a:misc xmlns:vehicle-a="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><bool-leaf xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">true</bool-leaf><empty-leaf xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"></empty-leaf><enum-leaf xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">a</enum-leaf><int-leaf xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">42</int-leaf><list-a xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" xc:operation="replace"><id xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">0</id><foo xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">asdf</foo></list-a><list-b xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" xc:operation="replace"><id xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">0</id></list-b><numbers xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" xc:operation="replace"><int8-leaf xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">0</int8-leaf><int16-leaf xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">0</int16-leaf><int32-leaf xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">0</int32-leaf><int64-leaf xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">0</int64-leaf><uint8-leaf xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">0</uint8-leaf><uint16-leaf xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">0</uint16-leaf><uint32-leaf xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">0</uint32-leaf><uint64-leaf xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">0</uint64-leaf><decimal-leaf xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">0</decimal-leaf></numbers><numbers xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" xc:operation="replace"><int8-leaf xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">1</int8-leaf><int16-leaf xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">0</int16-leaf><int32-leaf xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">0</int32-leaf><int64-leaf xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">0</int64-leaf><uint8-leaf xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">0</uint8-leaf><uint16-leaf xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">0</uint16-leaf><uint32-leaf xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">0</uint32-leaf><uint64-leaf xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">0</uint64-leaf><decimal-leaf xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">0</decimal-leaf></numbers></vehicle-a:misc></config>
        ''')

        schema = load_multiple_schema_root(["vehicle-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("PUT", url, (body,"json"))

        self.assertEquals(actual_xml, expected_xml)





########################################
# BEGIN BOILERPLATE JENKINS INTEGRATION

def main():                                                                      
    runner = xmlrunner.XMLTestRunner(output=os.environ["RIFT_MODULE_TEST"])      
    unittest.main(testRunner=runner)   

if __name__ == '__main__':
    main()

