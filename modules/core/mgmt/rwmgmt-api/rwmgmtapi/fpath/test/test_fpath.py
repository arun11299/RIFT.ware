
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import unittest
from lxml.etree import XML
from rwmgmtapi.fpath import fpath
from jinja2 import Template

CONFD_FPATH = Template("""
<collection
    xmlns:y="http://tail-f.com/ns/rest">
    {% for colony in colonies %}
    <colony
        xmlns="http://riftio.com/ns/riftware-1.0/rw-base">
        <name>{{colony.name}}</name>
        {% for context in colony.contexts %}
        <network-context>
            <name>{{context.name}}</name>
            {% for iface in context.interfaces %}
            <interface
                xmlns="http://riftio.com/ns/riftware-1.0/rw-fpath">
                <name>{{iface.name}}</name>
                <ip>
                    <address>11.0.1.4/24</address>
                </ip>
                {% for port in iface.ports %}
                <bind>
                    <port>{{port}}</port>
                </bind>
                {% endfor %}
            </interface>
            {% endfor %}
        </network-context>
        {% endfor %}
        {% for port in colony.ports %}
        <port
            xmlns="http://riftio.com/ns/riftware-1.0/rw-portconfig">
            <name>{{port}}</name>
            <detail>MOVE ME</detail>
        </port>
        {% endfor %}
    </colony>
    {% endfor %}
</collection>
""")

class TestBaseConfigBuilder(unittest.TestCase):

    ports = ['a', 'b']
    meta = {
        'colonies' : [{
            'name' : 'colony1',
            'contexts' : [{
                'name' : 'context1',
                'interfaces' : [{
                    'name' : 'iface1',
                    'ports' : ports
                }]
            }],
            'ports' : ports
        }]
    }

    def testAppendDetails(self):
        confd_fpath = XML(CONFD_FPATH.render(self.meta))
        #print(tostring(confd_fpath, pretty_print=True))
        b = fpath.BaseConfigBuilder()
        actual = b.append_interface_details(confd_fpath)
        #print(tostring(actual, pretty_print=True))
        iface_ports = actual.xpath('/network/network-context/interface/port')
        self.assertEqual(2, len(iface_ports))

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
