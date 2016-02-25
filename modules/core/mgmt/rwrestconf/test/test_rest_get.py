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

class TestRestGet(unittest.TestCase):
    # GET /api/config/top-container-deep
    def test_conversion_CONFD_URL_to_XML_00(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-container-deep"
        expected_xml = _collapse_string('''
<top-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><inner-list-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k /></inner-list-shallow><inner-container xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><inner-list xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k /></inner-list></inner-container><inner-list-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k /><inner-container-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"></inner-container-shallow><inner-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><bottom-list-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k /></bottom-list-shallow></inner-container-deep></inner-list-deep></top-container-deep>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    # GET /api/config/top-container-deep/a
    def test_conversion_CONFD_URL_to_XML_01(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-container-deep/a"
        expected_xml = _collapse_string('''
<top-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><a xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" /></top-container-deep>
        ''')
        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    # GET /api/config/top-container-deep/inner-list-deep
    def test_conversion_CONFD_URL_to_XML_02(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-container-deep/inner-list-deep"
        expected_xml = _collapse_string('''
<top-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><inner-list-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" /></top-container-deep>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    # GET /api/config/top-container-deep/inner-list-deep/some-key
    def test_conversion_CONFD_URL_to_XML_03(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-container-deep/inner-list-deep/some-key"
        expected_xml = _collapse_string('''
<top-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><inner-list-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k>some-key</k></inner-list-deep></top-container-deep>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    # GET /api/config/top-container-deep/inner-list-deep/some-key/inner-container-deep
    def test_conversion_CONFD_URL_to_XML_04(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-container-deep/inner-list-deep/some-key/inner-container-deep"
        expected_xml = _collapse_string('''
<top-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><inner-list-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k>some-key</k><inner-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" /></inner-list-deep></top-container-deep>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    # GET /api/config/top-container-deep/inner-list-deep/some-key/inner-container-deep/bottom-list-shallow
    def test_conversion_CONFD_URL_to_XML_05(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-container-deep/inner-list-deep/some-key/inner-container-deep/bottom-list-shallow"
        expected_xml = _collapse_string('''
<top-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><inner-list-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k>some-key</k><inner-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><bottom-list-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" /></inner-container-deep></inner-list-deep></top-container-deep>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    # GET /api/config/top-container-deep/inner-list-deep/some-key/inner-container-deep/bottom-list-shallow/some-key
    def test_conversion_CONFD_URL_to_XML_06(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-container-deep/inner-list-deep/some-key/inner-container-deep/bottom-list-shallow/some-key"
        expected_xml = _collapse_string('''
<top-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><inner-list-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k>some-key</k><inner-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><bottom-list-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k>some-key</k></bottom-list-shallow></inner-container-deep></inner-list-deep></top-container-deep>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    # GET /api/config/top-container-deep/inner-list-deep/some-key/inner-container-shallow
    def test_conversion_CONFD_URL_to_XML_07(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-container-deep/inner-list-deep/some-key/inner-container-shallow"
        expected_xml = _collapse_string('''
<top-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><inner-list-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k>some-key</k><inner-container-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" /></inner-list-deep></top-container-deep>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    # GET /api/config/top-container-deep/inner-list-deep/some-key/inner-container-shallow/a
    def test_conversion_CONFD_URL_to_XML_08(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-container-deep/inner-list-deep/some-key/inner-container-shallow/a"
        expected_xml = _collapse_string('''
<top-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><inner-list-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k>some-key</k><inner-container-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><a xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" /></inner-container-shallow></inner-list-deep></top-container-deep>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    # GET /api/config/top-container-deep/inner-list-shallow
    def test_conversion_CONFD_URL_to_XML_09(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-container-deep/inner-list-shallow"
        expected_xml = _collapse_string('''
<top-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><inner-list-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" /></top-container-deep>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    # GET /api/config/top-container-deep/inner-list-shallow/some-key
    def test_conversion_CONFD_URL_to_XML_10(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-container-deep/inner-list-shallow/some-key"
        expected_xml = _collapse_string('''
<top-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><inner-list-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k>some-key</k></inner-list-shallow></top-container-deep>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    # GET /api/config/top-container-shallow
    def test_conversion_CONFD_URL_to_XML_11(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-container-shallow"
        expected_xml = _collapse_string('''
<top-container-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"></top-container-shallow>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    # GET /api/config/top-container-shallow/a
    def test_conversion_CONFD_URL_to_XML_12(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-container-shallow/a"
        expected_xml = _collapse_string('''
<top-container-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><a xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" /></top-container-shallow>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    # GET /api/config/top-list-deep
    def test_conversion_CONFD_URL_to_XML_13(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-list-deep"
        expected_xml = _collapse_string('''
<top-list-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k /><inner-list xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k /><inner-container xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"></inner-container></inner-list><inner-container-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"></inner-container-shallow><inner-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><bottom-list-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k /></bottom-list-shallow></inner-container-deep></top-list-deep>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    # GET /api/config/top-list-deep/some-key
    def test_conversion_CONFD_URL_to_XML_14(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-list-deep/some-key"
        expected_xml = _collapse_string('''
<top-list-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k>some-key</k></top-list-deep>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    # GET /api/config/top-list-deep/some-key/inner-container-deep
    def test_conversion_CONFD_URL_to_XML_15(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-list-deep/some-key/inner-container-deep"
        expected_xml = _collapse_string('''
<top-list-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k>some-key</k><inner-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" /></top-list-deep>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    # GET /api/config/top-list-deep/some-key/inner-container-deep/bottom-list-shallow
    def test_conversion_CONFD_URL_to_XML_16(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-list-deep/some-key/inner-container-deep/bottom-list-shallow"
        expected_xml = _collapse_string('''
<top-list-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k>some-key</k><inner-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><bottom-list-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" /></inner-container-deep></top-list-deep>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    # GET /api/config/top-list-deep/some-key/inner-container-deep/bottom-list-shallow/some-key
    def test_conversion_CONFD_URL_to_XML_17(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-list-deep/some-key/inner-container-deep/bottom-list-shallow/some-key"
        expected_xml = _collapse_string('''
<top-list-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k>some-key</k><inner-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><bottom-list-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k>some-key</k></bottom-list-shallow></inner-container-deep></top-list-deep>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    # GET /api/config/top-list-deep/some-key/inner-container-shallow
    def test_conversion_CONFD_URL_to_XML_18(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-list-deep/some-key/inner-container-shallow"
        expected_xml = _collapse_string('''
<top-list-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k>some-key</k><inner-container-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" /></top-list-deep>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    # GET /api/config/top-list-deep/some-key/inner-container-shallow/a
    def test_conversion_CONFD_URL_to_XML_19(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-list-deep/some-key/inner-container-shallow/a"
        expected_xml = _collapse_string('''
<top-list-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k>some-key</k><inner-container-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><a xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" /></inner-container-shallow></top-list-deep>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    # GET /api/config/top-list-shallow
    def test_conversion_CONFD_URL_to_XML_20(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-list-shallow"
        expected_xml = _collapse_string('''
<top-list-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k /></top-list-shallow>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    # GET /api/config/top-list-shallow/some-key
    def test_conversion_CONFD_URL_to_XML_21(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-list-shallow/some-key"
        expected_xml = _collapse_string('''
<top-list-shallow xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k>some-key</k></top-list-shallow>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    def test_conversion_CONFD_URL_to_XML_22(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-list-deep/some-key/inner-list/a"
        expected_xml = _collapse_string('''
<top-list-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k>some-key</k><inner-list xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k>a</k></inner-list></top-list-deep>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    def test_conversion_CONFD_URL_to_XML_23(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-list-deep/some-key/inner-list/inner-container"
        expected_xml = _collapse_string('''
<top-list-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k>some-key</k><inner-list xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><k>inner-container</k></inner-list></top-list-deep>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    def test_conversion_CONFD_URL_to_XML_24(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/top-container-deep/inner-container"
        expected_xml = _collapse_string('''
<top-container-deep xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><inner-container xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" /></top-container-deep>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    def test_conversion_CONFD_URL_to_XML_25(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/multi-key/firstkey,secondkey/treasure"
        expected_xml = _collapse_string('''
<multi-key xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a"><foo>firstkey</foo><bar>secondkey</bar><treasure xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" /></multi-key>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    def test_conversion_CONFD_URL_to_XML_26(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/vehicle-augment-a:clash"
        expected_xml = _collapse_string('''
<clash xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-augment-a"></clash>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)


    def test_conversion_CONFD_URL_to_XML_27(self):
        self.maxDiff = None # expected_xml is too large

        url = "/api/config/vehicle-augment-a:clash/bar"
        expected_xml = _collapse_string('''
<vehicle-augment-a:clash xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-augment-a"><bar xmlns="http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-augment-a" /></vehicle-augment-a:clash>
        ''')

        schema = load_multiple_schema_root(["vehicle-a","vehicle-augment-a"])

        converter = ConfdRestTranslator(schema)

        actual_xml = converter.convert("GET", url, None)

        self.assertEquals(actual_xml, expected_xml)

    def test_negative_not_enough_keys(self):
        self.maxDiff = None
        url = "/api/config/multi-key/a"

        schema = load_multiple_schema_root(["vehicle-a"])
        converter = ConfdRestTranslator(schema)

        with self.assertRaises(ValueError):
            converter.convert("GET", url, None)

    def test_negative_too_many_keys(self):
        self.maxDiff = None
        url = "/api/config/multi-key/a,b,c"

        schema = load_multiple_schema_root(["vehicle-a"])
        converter = ConfdRestTranslator(schema)

        with self.assertRaises(ValueError):
            converter.convert("PUT", url, None)

    def test_negative_invalid_operation(self):
        self.maxDiff = None
        url = "/api/config/something"

        schema = load_multiple_schema_root(["vehicle-a"])
        converter = ConfdRestTranslator(schema)

        with self.assertRaises(ValueError):
            converter.convert("INVALID OPERATION", url, None)

    def test_negative_delete_container(self):
        self.maxDiff = None
        url = "/api/config/top-container-shallow"

        schema = load_multiple_schema_root(["vehicle-a"])
        converter = ConfdRestTranslator(schema)

        with self.assertRaises(ValueError):
            converter.convert("DELETE", url, None)

    def test_negative_bad_query(self):
        self.maxDiff = None
        url = "/api/config/top-container-shallow?invalid-query"

        schema = load_multiple_schema_root(["vehicle-a"])
        converter = ConfdRestTranslator(schema)

        with self.assertRaises(ValueError):
            converter.convert("GET", url, None)

    def test_negative_bad_query_select_option(self):
        self.maxDiff = None
        url = "/api/config/top-container-shallow?select=(bad use of parens)"

        schema = load_multiple_schema_root(["vehicle-a"])
        converter = ConfdRestTranslator(schema)

        with self.assertRaises(ValueError):
            converter.convert("GET", url, None)

########################################
# BEGIN BOILERPLATE JENKINS INTEGRATION

def main():                                                                      
    runner = xmlrunner.XMLTestRunner(output=os.environ["RIFT_MODULE_TEST"])      
    unittest.main(testRunner=runner)   

if __name__ == '__main__':
    main()

