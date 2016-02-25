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
    convert_netconf_response_to_json,
    convert_rpc_to_json_output,
    convert_rpc_to_xml_output,
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

class TestRest(unittest.TestCase):

    def test_conversion_PUT_JSON_to_XML_1(self):
        self.maxDiff = None

        url = '/api/config/car'
        json = _collapse_string('''
{
    "car":[
        {
            "brand":"subaru",
            "models":{
                "name-m":"WRX",
                "year":2015,
                "capacity":5,
                "is-cool":"True"
            }
        }
    ]
}
       ''')

        expected_xml = _collapse_string('''
<config xmlns:xc="urn:ietf:params:xml:ns:netconf:base:1.0"><car xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" xc:operation="replace"><brand xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">subaru</brand><models xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><name-m xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">WRX</name-m><year xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">2015</year><capacity xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">5</capacity><is-cool xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">true</is-cool></models></car></config>
        ''')

        root = load_schema_root("vehicle-a")

        converter = ConfdRestTranslator(root)

        actual_xml = converter.convert("PUT", url, (json,"application/data+json"))

        self.assertEqual(actual_xml, expected_xml)

    def test_conversion_POST_JSON_to_XML_1(self):
        self.maxDiff = None

        url = '/api/config/car'
        json = _collapse_string('''
{
    "car":[
        {
            "brand":"subaru",
            "models":{
                "name-m":"WRX",
                "year":2015,
                "capacity":5,
                "is-cool":"True"
            }
        }
    ]
}
        ''')

        expected_xml = _collapse_string('''
<config xmlns:xc="urn:ietf:params:xml:ns:netconf:base:1.0"><car xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" xc:operation="create"><brand xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">subaru</brand><models xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><name-m xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">WRX</name-m><year xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">2015</year><capacity xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">5</capacity><is-cool xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">true</is-cool></models></car></config>
        ''')

        root = load_schema_root("vehicle-a")

        converter = ConfdRestTranslator(root)

        actual_xml = converter.convert("POST", url, (json,"application/data+json"))

        self.assertEqual(actual_xml, expected_xml)


    def test_conversion_CONFD_URL_to_XML_1(self):
        self.maxDiff = None # expected_xml is too large
        # trailing '/' is on purpose to test that edge case
        url = "/api/operational/car/subaru/models/" 
        expected_xml = _collapse_string('''
<car xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><brand>subaru</brand><models xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" /></car>
        ''')

        root = load_schema_root("vehicle-a")
        
        converter = ConfdRestTranslator(root)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    def test_conversion_CONFD_URL_to_XML_2(self):
        self.maxDiff = None # expected_xml is too large
        url = "/api/operational/whatever/inner-whatever/list-whatever"
        expected_xml = _collapse_string('''
<whatever xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><inner-whatever xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><list-whatever xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" /></inner-whatever></whatever>
        ''')

        root = load_schema_root("vehicle-a")
        
        converter = ConfdRestTranslator(root)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    def test_conversion_CONFD_URL_to_XML_3(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/operational/car/subaru/extras?select=speakers;engine(*)"
        expected_xml = _collapse_string('''
<car xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">
  <brand>
  subaru</brand>
  <extras xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-augment-a">
    <engine xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-augment-a" />
    <speakers />
  </extras>
</car>

        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    def test_conversion_CONFD_URL_to_XML_4(self):
        self.maxDiff = None # expected_xml is too large
        url = "/api/operational/whatever/inner-whatever?deep"
        expected_xml = _collapse_string('''
<whatever xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><inner-whatever xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" /></whatever>
        ''')

        root = load_schema_root("vehicle-a")
        
        converter = ConfdRestTranslator(root)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)


    def test_conversion_CONFD_URL_to_XML_5(self):
        self.maxDiff = None # expected_xml is too large
        url = "/api/operational/whatever/inner-whatever"
        expected_xml = _collapse_string('''
<whatever xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><inner-whatever xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" /></whatever>
        ''')

        root = load_schema_root("vehicle-a")
        
        converter = ConfdRestTranslator(root)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    def test_conversion_CONFD_URL_to_XML_6(self):
        self.maxDiff = None # expected_xml is too large
        url = "/api/operational/car?select=brand"
        expected_xml = _collapse_string('''
<car xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><brand /></car>
        ''')

        root = load_schema_root("vehicle-a")
        
        converter = ConfdRestTranslator(root)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    def test_conversion_CONFD_URL_to_XML_get_car_deep(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/operational/car?deep"
        expected_xml = _collapse_string('''
<car xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" />
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    def test_conversion_CONFD_URL_to_XML_get_car(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/operational/car"
        expected_xml = _collapse_string('''
<car xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><brand /><models xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><name-m /></models><extras xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-augment-a"><name-e /><engine xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-augment-a"></engine><features xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-augment-a"><package /><items xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-augment-a"><name /></items></features></extras></car>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    def test_conversion_CONFD_URL_to_XML_get_list_key(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/operational/car/honda"
        expected_xml = _collapse_string('''
<car xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><brand>honda</brand></car>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    def test_conversion_CONFD_URL_to_XML_get_bogus(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/operational/bogus"

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        caught_exception = False
        try:
            actual_xml = converter.convert("GET", url, None)
        except ValueError:
            caught_exception = True

        self.assertTrue(caught_exception)

    def test_conversion_CONFD_URL_to_XML_delete_list_key(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/operational/top-container-deep/inner-container/inner-list/some-key"
        expected_xml = _collapse_string('''
<config xmlns:xc="urn:ietf:params:xml:ns:netconf:base:1.0"><top-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><inner-container xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><inner-list xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" xc:operation="delete"><k>some-key</k></inner-list></inner-container></top-container-deep></config>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("DELETE", url, None)

        self.assertEquals(actual_xml, expected_xml)

    def test_conversion_CONFD_XML_to_JSON_convert_error_1(self):
        self.maxDiff = None

        xml = _collapse_string('''
<rpc-reply xmlns="urn:ietf:params:xml:ns:netconf:base:1.0" message-id="urn:uuid:2bdd6476-92d4-11e5-86ca-fa163e622f12" xmlns:nc="urn:ietf:params:xml:ns:netconf:base:1.0">
  <rpc-error>
  <error-type>application</error-type>
  <error-tag>data-missing</error-tag>
  <error-severity>error</error-severity>
  <error-path xmlns:rw-mc="http://riftio.com/ns/riftware-1.0/rw-mc" xmlns:nc="urn:ietf:params:xml:ns:netconf:base:1.0">
    /nc:rpc/nc:edit-config/nc:config/rw-mc:cloud/rw-mc:account[rw-mc:name=\'cloudname\']
  </error-path>
  <error-info>
    <bad-element>account</bad-element>
  </error-info>
</rpc-error>
</rpc-reply>
        ''')

        expected_json = _collapse_string('''
{"rpc-reply" : {"rpc-error" : {"error-type" : "application","error-tag" : "data-missing","error-severity" : "error","error-path" : "/nc:rpc/nc:edit-config/nc:config/rw-mc:cloud/rw-mc:account[rw-mc:name='cloudname']","error-info" : {"bad-element" : "account"}}}}
        ''')

        actual_json = convert_netconf_response_to_json(bytes(xml,"utf-8"))

        actual = _ordered(json.loads(actual_json))
        expected = _ordered(json.loads(expected_json))

        self.assertEquals(actual, expected)

    def test_conversion_CONFD_XML_to_JSON_convert_error_2(self):
        self.maxDiff = None

        xml = _collapse_string('''
<?xml version="1.0" encoding="UTF-8"?>
<rpc-reply xmlns="urn:ietf:params:xml:ns:netconf:base:1.0" message-id="urn:uuid:b5b5fd30-8eeb-11e5-8001-fa163e05ddfa" xmlns:nc="urn:ietf:params:xml:ns:netconf:base:1.0"><rpc-error>
<error-type>application</error-type>
<error-tag>data-missing</error-tag>
<error-severity>error</error-severity>
<error-app-tag>instance-required</error-app-tag>
<error-path xmlns:rw-mc="http://riftio.com/ns/riftware-1.0/rw-mc">
    /rpc/edit-config/config/rw-mc:network-pool/rw-mc:pool[rw-mc:name='n1']/rw-mc:cloud-account
</error-path>
<error-message xml:lang="en">illegal reference /network-pool/pool[name='n1']/cloud-account</error-message>
<error-info xmlns:tailf="http://tail-f.com/ns/netconf/params/1.1"  xmlns:rw-mc="http://riftio.com/ns/riftware-1.0/rw-mc">
  <tailf:bad-keyref>
    <tailf:bad-element>/rw-mc:network-pool/rw-mc:pool[rw-mc:name='n1']/rw-mc:cloud-account</tailf:bad-element>
    <tailf:missing-element>/rw-mc:cloud/rw-mc:account[rw-mc:name='c1']</tailf:missing-element>
  </tailf:bad-keyref>
</error-info>
</rpc-error>
</rpc-reply>
        ''')

        expected_json = _collapse_string('''
{"rpc-reply" : {"rpc-error" : {"error-type" : "application","error-tag" : "data-missing","error-severity" : "error","error-app-tag" : "instance-required","error-path" : "/rpc/edit-config/config/rw-mc:network-pool/rw-mc:pool[rw-mc:name='n1']/rw-mc:cloud-account","error-message" : "illegal reference /network-pool/pool[name='n1']/cloud-account","error-info" : {"bad-keyref" : {"bad-element" : "/rw-mc:network-pool/rw-mc:pool[rw-mc:name='n1']/rw-mc:cloud-account","missing-element" : "/rw-mc:cloud/rw-mc:account[rw-mc:name='c1']"}}}}}
        ''')

        actual_json = convert_netconf_response_to_json(bytes(xml,"utf-8"))

        actual = _ordered(json.loads(actual_json))
        expected = _ordered(json.loads(expected_json))

        self.assertEquals(actual, expected)


    def test_conversion_CONFD_XML_to_JSON_convert_error_3(self):
        self.maxDiff = None

        xml = _collapse_string('''
<?xml version="1.0" encoding="UTF-8"?>
<rpc-reply xmlns="urn:ietf:params:xml:ns:netconf:base:1.0" message-id="urn:uuid:ab56ec5e-92e3-11e5-83c5-fa163e622f12" xmlns:nc="urn:ietf:params:xml:ns:netconf:base:1.0">
    <ok/>
</rpc-reply>
        ''')

        expected_json = _collapse_string('''
        {"rpc-reply" : {"ok" : ""}}
        ''')

        actual_json = convert_netconf_response_to_json(bytes(xml,"utf-8"))

        actual = _ordered(json.loads(actual_json))
        expected = _ordered(json.loads(expected_json))

        self.assertEquals(actual, expected)

    def test_conversion_CONFD_XML_to_JSON_convert_error_4(self):
        self.maxDiff = None

        xml = _collapse_string('''
 <?xml version="1.0" encoding="UTF-8"?>

<rpc-reply xmlns="urn:ietf:params:xml:ns:netconf:base:1.0" message-id="urn:uuid:c7c92ac8-921b-11e5-8155-fa163e05ddfa" xmlns:nc="urn:ietf:params:xml:ns:netconf:base:1.0"><rpc-error>

<error-type>application</error-type>

<error-tag>data-missing</error-tag>

<error-severity>error</error-severity>

<error-app-tag>instance-required</error-app-tag>

<error-path xmlns:rw-mc="http://riftio.com/ns/riftware-1.0/rw-mc">

    /rpc/edit-config/config/rw-mc:mgmt-domain/rw-mc:domain[rw-mc:name='m1']/rw-mc:pools/rw-mc:network[rw-mc:name='n1']/rw-mc:name

  </error-path><error-message xml:lang="en">illegal reference /mgmt-domain/domain[name='m1']/pools/network[name='n1']/name</error-message><error-info xmlns:tailf="http://tail-f.com/ns/netconf/params/1.1"  xmlns:rw-mc="http://riftio.com/ns/riftware-1.0/rw-mc">

<tailf:bad-keyref>

<tailf:bad-element>/rw-mc:mgmt-domain/rw-mc:domain[rw-mc:name='m1']/rw-mc:pools/rw-mc:network[rw-mc:name='n1']/rw-mc:name</tailf:bad-element>

<tailf:missing-element>/rw-mc:network-pool/rw-mc:pool[rw-mc:name='n1']</tailf:missing-element>

</tailf:bad-keyref>

</error-info>

</rpc-error>

</rpc-reply>

        ''')

        expected_json = _collapse_string('''
{"rpc-reply" : {"rpc-error" : {"error-type" : "application","error-tag" : "data-missing","error-severity" : "error","error-app-tag" : "instance-required","error-path" : "/rpc/edit-config/config/rw-mc:mgmt-domain/rw-mc:domain[rw-mc:name='m1']/rw-mc:pools/rw-mc:network[rw-mc:name='n1']/rw-mc:name","error-message" : "illegal reference /mgmt-domain/domain[name='m1']/pools/network[name='n1']/name","error-info" : {"bad-keyref" : {"bad-element" : "/rw-mc:mgmt-domain/rw-mc:domain[rw-mc:name='m1']/rw-mc:pools/rw-mc:network[rw-mc:name='n1']/rw-mc:name","missing-element" : "/rw-mc:network-pool/rw-mc:pool[rw-mc:name='n1']"}}}}}
        ''')

        actual_json = convert_netconf_response_to_json(bytes(xml,"utf-8"))

        actual = _ordered(json.loads(actual_json))
        expected = _ordered(json.loads(expected_json))

        self.assertEquals(actual, expected)

    # POST /api/config/top-container-deep/inner-list-deep
    def test_conversion_post_container_list_1(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-container-deep/inner-list-deep"
        expected_xml = _collapse_string('''
<config xmlns:xc="urn:ietf:params:xml:ns:netconf:base:1.0"><top-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><inner-list-deep xc:operation="create"><k>asdf</k><inner-container-shallow><a>fdsa</a></inner-container-shallow></inner-list-deep></top-container-deep></config>
        ''')

        body = _collapse_string('''
<inner-list-deep>
<k>asdf</k>
<inner-container-shallow>
  <a>fdsa</a>
</inner-container-shallow>
</inner-list-deep>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("POST", url, (body,"xml"))

        self.assertEquals(actual_xml, expected_xml)

    # POST /api/config/top-container-deep/inner-list-deep
    def test_conversion_post_container_list_2(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-container-deep"
        expected_xml = _collapse_string('''
<config xmlns:xc="urn:ietf:params:xml:ns:netconf:base:1.0"><vehicle-a:top-container-deep xmlns:vehicle-a="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><top-container-deep xc:operation="create"><a>asdf</a><inner-list-deep><k>asdf</k><inner-container-shallow><a>fdsa</a></inner-container-shallow></inner-list-deep></top-container-deep></vehicle-a:top-container-deep></config>
        ''')

        body = _collapse_string('''
<top-container-deep>
<a>asdf</a>
<inner-list-deep>
  <k>asdf</k>
    <inner-container-shallow>
    <a>fdsa</a>
  </inner-container-shallow>
</inner-list-deep>
</top-container-deep>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("POST", url, (body,"xml"))

        self.assertEquals(actual_xml, expected_xml)

    # POST /api/config/top-container-deep/inner-list-deep
    def test_conversion_post_container_list_3(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-container-deep/inner-container"
        expected_xml = _collapse_string('''
<config xmlns:xc="urn:ietf:params:xml:ns:netconf:base:1.0"><top-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><vehicle-a:inner-container xmlns:vehicle-a="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><inner-container xc:operation="create"><a>fdsa</a></inner-container></vehicle-a:inner-container></top-container-deep></config>
        ''')

        body = _collapse_string('''
<inner-container>
  <a>fdsa</a>
</inner-container>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("POST", url, (body,"xml"))

        self.assertEquals(actual_xml, expected_xml)

    def test_conversion_rpc_to_xml_output(self):
        self.maxDiff = None
        xml = _collapse_string('''
<?xml version="1.0" encoding="UTF-8"?>
<rpc-reply xmlns="urn:ietf:params:xml:ns:netconf:base:1.0" message-id="urn:uuid:0ba69bd8-938c-11e5-a933-fa163e622f12" xmlns:nc="urn:ietf:params:xml:ns:netconf:base:1.0">
    <out xmlns="http://riftio.com/ns/riftware-1.0/rw-restconf">true</out>
</rpc-reply>
        ''')

        expected_xml = _collapse_string('''
<output><out xmlns="http://riftio.com/ns/riftware-1.0/rw-restconf">true</out></output>
        ''')

        actual_xml = convert_rpc_to_xml_output(xml)

        self.assertEqual(actual_xml, expected_xml)

    def test_conversion_rpc_to_xml_output_1(self):
        self.maxDiff = None
        xml = _collapse_string('''
<?xml version="1.0" encoding="UTF-8"?>\n<rpc-reply xmlns="urn:ietf:params:xml:ns:netconf:base:1.0" message-id="urn:uuid:0fc67048-93c4-11e5-bfc9-fa163e622f12" xmlns:nc="urn:ietf:params:xml:ns:netconf:base:1.0"><out1 xmlns="http://riftio.com/ns/riftware-1.0/rw-restconf">hello</out1>\n<out2 xmlns="http://riftio.com/ns/riftware-1.0/rw-restconf">world</out2>\n</rpc-reply>
        ''')

        expected_xml = _collapse_string('''
<output><out1 xmlns="http://riftio.com/ns/riftware-1.0/rw-restconf">hello</out1><out2 xmlns="http://riftio.com/ns/riftware-1.0/rw-restconf">world</out2></output>
        ''')

        actual_xml = convert_rpc_to_xml_output(xml)

        self.assertEqual(actual_xml, expected_xml)

    def test_conversion_rpc_to_json_output(self):
        self.maxDiff = None
        url = "/api/operations/ping"
        xml = _collapse_string('''
<?xml version="1.0" encoding="UTF-8"?>
<rpc-reply xmlns="urn:ietf:params:xml:ns:netconf:base:1.0" message-id="urn:uuid:0ba69bd8-938c-11e5-a933-fa163e622f12" xmlns:nc="urn:ietf:params:xml:ns:netconf:base:1.0">
    <out xmlns='http://riftio.com/ns/riftware-1.0/rw-restconf'>true</out>
</rpc-reply>
        ''')

        expected_json = _collapse_string('''
{"output": {"out": "true"}}
        ''')

        intermediate_json = convert_netconf_response_to_json(bytes(xml,"utf-8"))

        actual_json = convert_rpc_to_json_output(intermediate_json)

        actual = _ordered(json.loads(actual_json))
        expected = _ordered(json.loads(expected_json))

        self.assertEqual(actual, expected)

    def test_conversion_XML_to_JSON_1(self):
        self.maxDiff = None
        url = "/api/operational/car/toyota/models"
        xml = _collapse_string('''
        <data>
          <car xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">
            <models>
              <name-m>Camry</name-m>	  
              <year>2015</year>	  
              <capacity>5</capacity>	  
            </models>
          </car>
        </data>
        ''')

        expected_json = _collapse_string('''
{"vehicle-a:models":[{"year" : 2015,"name-m" : "Camry","capacity" : 5}]}
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = XmlToJsonTranslator(schema)

        xpath = create_xpath_from_url(url, schema)
        
        actual_json = converter.convert(False, url, xpath, xml)

        actual = _ordered(json.loads(actual_json))
        expected = _ordered(json.loads(expected_json))

        self.assertEquals(actual, expected)

    def test_conversion_XML_to_JSON_2(self):
        self.maxDiff = None
        url = "/api/operational/car/toyota/models"
        xml = _collapse_string('''
        <data>
          <car xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">
            <models>
              <name-m>Camry</name-m>	  
              <year>2015</year>	  
              <capacity>5</capacity>	  
            </models>
            <models>
              <name-m>Tacoma</name-m>	  
              <year>2017</year>	  
              <capacity>7</capacity>	  
            </models>
          </car>
        </data>
        ''')

        expected_json = _collapse_string('''
{"collection":{"vehicle-a:models":[{"name-m" : "Camry","capacity" : 5,"year" : 2015},{"name-m" : "Tacoma","capacity" : 7,"year" : 2017}]}}
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = XmlToJsonTranslator(schema)

        xpath = create_xpath_from_url(url, schema)
        
        actual_json = converter.convert(True, url, xpath, xml)

        actual = _ordered(json.loads(actual_json))
        expected = _ordered(json.loads(expected_json))

        self.assertEquals(actual, expected)

    def test_conversion_XML_to_JSON_3(self):
        self.maxDiff = None
        url = "/api/operational/cloud/account/"
        xml = _collapse_string('''
<data>
<cloud xmlns="http://riftio.com/ns/riftware-1.0/rw-mc">
  <account>
    <openstack>
      <key>pluto</key>
      <secret>mypasswd</secret>
      <auth_url>http://10.66.4.18:5000/v3/</auth_url>
      <tenant>demo</tenant>      
      <mgmt-network>private</mgmt-network>      
    </openstack>
  </account>
</cloud>
</data>
        ''')

        expected_json = _collapse_string('''
{"rw-mc:account":[{"openstack":{"auth_url" : "http://10.66.4.18:5000/v3/","tenant" : "demo","key" : "pluto","mgmt-network" : "private","secret" : "mypasswd"}}]}
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a", "rw-mc"])

        converter = XmlToJsonTranslator(schema)

        xpath = create_xpath_from_url(url, schema)
        
        actual_json = converter.convert(False, url, xpath, xml)

        actual = _ordered(json.loads(actual_json))
        expected = _ordered(json.loads(expected_json))

        self.assertEquals(actual, expected)

    def test_conversion_JSON_to_xml_xy(self):
        self.maxDiff = None
        url = "/api/running/cloud/account/OS"
        body = _collapse_string('''
{
  "rw-mc:account": {
	"name": "OS",
	"account-type": "openstack",
    "openstack": {
      "key": "demo",
      "secret": "mypasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddswd",
      "auth_url": "http://10.66.4.2:5000/v3/",
      "tenant":"demo",
      "mgmt_network": "private"
    }
  }
}
        ''')

        expected_xml = _collapse_string('''
<config xmlns:xc="urn:ietf:params:xml:ns:netconf:base:1.0"><cloud xmlns="http://riftio.com/ns/riftware-1.0/rw-mc"><account xmlns="http://riftio.com/ns/riftware-1.0/rw-mc"><name xmlns="http://riftio.com/ns/riftware-1.0/rw-mc">OS</name><account-type xmlns="http://riftio.com/ns/riftware-1.0/rw-mc">openstack</account-type><openstack xmlns="http://riftio.com/ns/riftware-1.0/rw-mc"><key xmlns="http://riftio.com/ns/riftware-1.0/rw-mc">demo</key><secret xmlns="http://riftio.com/ns/riftware-1.0/rw-mc">mypasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddswd</secret><auth_url xmlns="http://riftio.com/ns/riftware-1.0/rw-mc">http://10.66.4.2:5000/v3/</auth_url><tenant xmlns="http://riftio.com/ns/riftware-1.0/rw-mc">demo</tenant></openstack></account></cloud></config>
        ''')

        schema = load_multiple_schema_root(["rw-mc"])
        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("PUT", url, (body,"json"))

        self.assertEquals(actual_xml, expected_xml)

    def test_conversion_XML_to_JSON_5(self):
        self.maxDiff = None
        url = "/api/operational/cloud/account/OS"
        xml = _collapse_string('''
<data>
<cloud xmlns="http://riftio.com/ns/riftware-1.0/rw-mc" xmlns:nc="urn:ietf:params:xml:ns:netconf:base:1.0">
  <account>
    <name>OS</name>
    <account-type>openstack</account-type>
    <openstack>
      <key>pluto</key>
      <secret>mypasswd</secret>
      <auth_url>http://10.66.4.18:5000/v3/</auth_url>
      <tenant>demo</tenant>
      <mgmt-network>private</mgmt-network>
    </openstack>
  </account>
</cloud>
</data>

        ''')

        expected_json = _collapse_string('''
{"rw-mc:account" :{"openstack":{"tenant" : "demo","secret" : "mypasswd","auth_url" : "http://10.66.4.18:5000/v3/","mgmt-network" : "private","key" : "pluto"},"name" : "OS","account-type" : "openstack"}}
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a", "rw-mc"])

        converter = XmlToJsonTranslator(schema)

        xpath = create_xpath_from_url(url, schema)

        actual_json = converter.convert(False, url, xpath, xml)

        actual = _ordered(json.loads(actual_json))
        expected = _ordered(json.loads(expected_json))

        self.assertEquals(actual, expected)

    def test_conversion_XML_to_JSON_6(self):
        self.maxDiff = None
        url = "/api/running/logging/syslog-viewer"
        xml = _collapse_string('''
<data>
<logging xmlns="http://riftio.com/ns/riftware-1.0/rwlog-mgmt" xmlns:nc="urn:ietf:params:xml:ns:netconf:base:1.0">
    <syslog-viewer>http://10.0.119.5/loganalyzer</syslog-viewer>
</logging>
</data>
        ''')

        expected_json = _collapse_string('''
{"rwlog-mgmt:syslog-viewer" : "http://10.0.119.5/loganalyzer"}
        ''')

        schema = load_multiple_schema_root(["rwlog-mgmt"])

        converter = XmlToJsonTranslator(schema)

        xpath = create_xpath_from_url(url, schema)

        actual_json = converter.convert(False, url, xpath, xml)

        actual = _ordered(json.loads(actual_json))
        expected = _ordered(json.loads(expected_json))

        self.assertEquals(actual, expected)

    def test_conversion_post(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-container-deep/inner-list-shallow"
        expected_xml = _collapse_string('''
<config xmlns:xc="urn:ietf:params:xml:ns:netconf:base:1.0"><top-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><inner-list-shallow xmlns="http://riftio.com/ns/example" xc:operation="create"><k>asdf</k></inner-list-shallow></top-container-deep></config>
        ''')

        body = _collapse_string('''
<inner-list-shallow xmlns="http://riftio.com/ns/example">
        <k>asdf</k>
</inner-list-shallow>
        ''')

        schema = load_multiple_schema_root(["vehicle-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("POST", url, (body,"xml"))

        self.assertEquals(actual_xml, expected_xml)

    def test_conversion_XML_to_JSON_null_string(self):
        self.maxDiff = None
        url = "/api/operational/car/toyota/models"
        xml = _collapse_string('''
        <data>
          <car xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">
            <models>
              <name-m/>
            </models>
          </car>
        </data>
        ''')

        expected_json = _collapse_string('''
        {"vehicle-a:models":[{"name-m" : ""}]}
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = XmlToJsonTranslator(schema)

        xpath = create_xpath_from_url(url, schema)
        
        actual_json = converter.convert(False, url, xpath, xml)

        actual = _ordered(json.loads(actual_json))
        expected = _ordered(json.loads(expected_json))

        self.assertEquals(actual, expected)


    def test_conversion_POST_JSON_to_XML_asdf(self):
        self.maxDiff = None

        url = '/api/operations/exec-ns-config-primitive'
        json = _collapse_string('''
{
    "input":{
        "vehicle-a:name":"create-user",
        "vehicle-a:nsr_id_ref":"1181c5ce-9c8b-4e1d-9247-2a38cdf128bb",
        "vehicle-a:vnf-primitive-group":[
            {
                "member-vnf-index-ref":"1",
                "vnfr-id-ref":"76d116e8-cf66-11e5-841e-6cb3113b406f",
                "primitive":[
                    {
                        "name":"number",
                        "value":"123"
                    },
                    {
                        "name":"password",
                        "value":"letmein"
                    }
                ]
            }
        ]
    }
}
        ''')

        expected_xml = _collapse_string('''
<vehicle-a:exec-ns-config-primitive xmlns:vehicle-a="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><name xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">create-user</name><nsr_id_ref xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">1181c5ce-9c8b-4e1d-9247-2a38cdf128bb</nsr_id_ref><vnf-primitive-group xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><member-vnf-index-ref xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">1</member-vnf-index-ref><vnfr-id-ref xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">76d116e8-cf66-11e5-841e-6cb3113b406f</vnfr-id-ref><primitive xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><name xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">number</name><value xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">123</value></primitive><primitive xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><name xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">password</name><value xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a">letmein</value></primitive></vnf-primitive-group></vehicle-a:exec-ns-config-primitive>
        ''')

        root = load_schema_root("vehicle-a")

        converter = ConfdRestTranslator(root)

        actual_xml = converter.convert("POST", url, (json,"application/data+json"))

        self.assertEqual(actual_xml, expected_xml)


########################################
# BEGIN BOILERPLATE JENKINS INTEGRATION

def main():                                                                      
    runner = xmlrunner.XMLTestRunner(output=os.environ["RIFT_MODULE_TEST"])      
    unittest.main(testRunner=runner)   

if __name__ == '__main__':
    main()
