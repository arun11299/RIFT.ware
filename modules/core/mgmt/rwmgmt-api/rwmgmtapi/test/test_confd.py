
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import unittest
from lxml import etree

from jinja2 import Template

from rwmgmtapi import confd
# from rwmgmtapi.test.xmlcompare import xml_compare


QUERY = Template("""
<start-query xmlns="http://tail-f.com/ns/tailf-rest-query">
    <foreach>{{foreach}}</foreach>
    {% for select in selects %}
    <select>
        <expression>{{select.expression}}</expression>
        <result-type>{{select.result_type}}</result-type>
        <label>{{select.label}}</label>
    </select>
    {% endfor %}
</start-query>
""")
class TestQueryBuilder(unittest.TestCase):

    def testQueryBuilder(self):
        actual = confd.QueryBuilder()
        actual.query('x')
        actual.select('e', 'r', 'l')
        expected_data = {
            'foreach' : 'x',
            'selects' : [{
                'expression' : 'e',
                'result_type' : 'r',
                'label' : 'l'
            }]
        }
        expected = etree.XML(QUERY.render(expected_data))
        def reporter(z):
            self.fail(z)
        print(expected)
        #  Not in OS
        # xml_compare(actual.xml, expected, reporter)

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
