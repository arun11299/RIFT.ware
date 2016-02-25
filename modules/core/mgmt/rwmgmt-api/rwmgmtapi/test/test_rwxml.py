
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

from rwmgmtapi import rwxml
import unittest


class TestRwXml(unittest.TestCase):

    def testBuilder(self):
        rdr = rwxml.Builder()
        actual = rdr.fpath('foo').fpath('bar').xml
        # print(tostring(actual, pretty_print=True))

    def testClean(self):
        self.assertEqual("hey", rwxml.clean("hey"))
        self.assertEqual("hey\n\r\tho", rwxml.clean("hey\n\r\tho"))
        self.assertEqual("hey ho", rwxml.clean("hey\x00ho"))