
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import unittest
import StringIO
import json

from rwmgmtapi.xml2json import XmlDecoder


class TestApp(unittest.TestCase):
    
    def setUp(self):
        self.decoder = XmlDecoder()

    def testSingleElement(self):
        xml = StringIO.StringIO('<e><a>text1</a></e>')
        root = self.decoder.decode(xml)
        actual = json.dumps(root)
        expected = '{"e": {"a": "text1"}}'
        self.assertEqual(expected, actual)

    def testSimpleArrayAndAttrs(self):
        xml = StringIO.StringIO('<e b="z"><a>text1</a><a>text2</a></e>')
        self.decoder.strategy.is_list = ['a']
        self.decoder.strategy.has_attributes = ['e']
        root = self.decoder.decode(xml)
        actual = json.dumps(root)
        expected = '{"e": {"@b": "z", "a": ["text1", "text2"]}}'
        self.assertEqual(expected, actual)

    def testComplexArrayAndAttrs(self):
        xml = StringIO.StringIO('<e b="z"><a c="d">text</a></e>')
        self.decoder.strategy.is_list = ['a']
        self.decoder.strategy.has_attributes = ['e', 'a']
        root = self.decoder.decode(xml)
        actual = json.dumps(root)
        expected = '{"e": {"@b": "z", "a": [{"@c": "d", "#content": "text"}]}}'
        self.assertEqual(expected, actual)

    def testMultipleArrayElementsAndAttrs(self):
        xml = StringIO.StringIO('<e b="z"><a c="d">text1</a><a c="e">text2</a></e>')
        self.decoder.strategy.is_list = ['a']
        self.decoder.strategy.has_attributes = ['e', 'a']
        root = self.decoder.decode(xml)
        actual = json.dumps(root)
        expected = '{"e": {"@b": "z", "a": [{"@c": "d", "#content": "text1"}, {"@c": "e", "#content": "text2"}]}}'
        self.assertEqual(expected, actual)

    def testHeterogenousElements(self):
        xml = StringIO.StringIO('<a><b><c>text1</c><d>text2</d></b></a>')
        root = self.decoder.decode(xml)
        actual = json.dumps(root)
        expected = '{"a": {"b": {"c": "text1", "d": "text2"}}}'
        self.assertEqual(expected, actual)

    # NOT FULLY SUPPORTED AT THIS TIME
    # having text nodes and elements under one parent may be 
    # common in HTML, but unlikely in config data so this mode
    # is not supported ATM.  If you really want to do this, if
    # parent node claims to have attributes even though it may not
    # then it would effectively work
    def testElementsWithTextAndElements(self):
        xml = StringIO.StringIO('<e><a>text1<x>y</x></a></e>')

        # This only works if 'a' claims to have attributes
        self.decoder.strategy.has_attributes = ['a']

        root = self.decoder.decode(xml)
        actual = json.dumps(root)
        expected = '{"e": {"a": {"#content": "text1", "x": "y"}}}'
        self.assertEqual(expected, actual)

    def testDoubleArray(self):
        xml = StringIO.StringIO("""
<a>
  <b>
    <c>a/b0/c0</c>
    <c>a/b0/c1</c>
  </b>
  <b>
    <c>a/b0/c0</c>
    <c>a/b0/c1</c>
  </b>
</a>
""")
        self.decoder.strategy.is_list = ['b', 'c']
        root = self.decoder.decode(xml)
        actual = json.dumps(root)
        expected = '{"a": {"b": [{"c": ["a/b0/c0", "a/b0/c1"]}, {"c": ["a/b0/c0", "a/b0/c1"]}]}}'
        self.assertEqual(expected, actual)

    def testEscaping(self):
        xml = StringIO.StringIO('<e><a>double quote begin"end</a></e>')
        root = self.decoder.decode(xml)
        actual = json.dumps(root)
        expected = '{"e": {"a": "double quote begin\\"end"}}'
        self.assertEqual(expected, actual)

    def testEscaping(self):
        xml = StringIO.StringIO("""
<a>
line 1
line 2
</a>
""")
        root = self.decoder.decode(xml)
        actual = json.dumps(root)
        expected = '{"a": "line 1\\nline 2"}'
        self.assertEqual(expected, actual)

########################################
# BEGIN BOILERPLATE JENKIN INTEGRATION
import logging
import xmlrunner
import argparse
import sys
import os
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
