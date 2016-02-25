
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import unittest
from rwmgmtapi.stats import Stats
import time
import logging

class TestStats(unittest.TestCase):

    def test_StartEnd(self):
        stats = Stats('x')
        token = stats.record_start('x')
        time.sleep(0.001)
        stats.record_end(token)
        self.assertEqual(1, stats.n_requests)
        self.assertTrue(stats.max_request >= 0.001)

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
