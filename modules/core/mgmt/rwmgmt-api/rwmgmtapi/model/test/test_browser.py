
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import unittest
from rwmgmtapi.model import browser
import abc
import json

class TestBrowser(unittest.TestCase):

    def _test_tree(self):
        s = browser.Schema()
        b = browser.Browser('rw-fpath')
        b.walk(s)
        print(json.dumps(s.root, indent=4, separators=(',', ': ')))

    def test_filter(self):
        s = browser.Schema(keep_empty_collections=False)
        b = browser.Browser('rw-fpath')
        b.walk(s, browser.ConfigOnly())
        print(json.dumps(s.root, indent=4, separators=(',', ': ')))

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
