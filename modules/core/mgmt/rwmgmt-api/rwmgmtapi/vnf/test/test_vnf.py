
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import unittest
from lxml.etree import tostring, XML
from rwmgmtapi.vnf import vnf
from jinja2 import Template


VCS = Template("""
<sector>
  <colony>
    <cluster>
    {% for vm in vms %}
      <vm>
        <component_name>{{vm.name}}</component_name>
      </vm>
    {% endfor %}
    </cluster>
  </colony>
</sector>
""")

class TestVcsConverter(unittest.TestCase):

    meta = {
        'vms' : [
            { 'name' : 'TrafGenA' },
            { 'name' : 'TrafGenB' },
            { 'name' : 'TrafSimClientA' },
            { 'name' : 'TrafSimServerB' },
            { 'name' : 'UnidentifiedA' }
        ]
    }

    def testConvert(self):
        cvtr = vnf.VcsConverter()
        vcs = XML(VCS.render(self.meta))
        #print(tostring(vcs, pretty_print=True))
        actual = cvtr.convert(vcs)
        #print(tostring(actual, pretty_print=True))
        vnfs = actual.xpath('/vnfs/vnf')
        self.assertEqual(3, len(vnfs))

VNF = Template("""
<vnfs>
  {% for vnf in vnfs %}
  <vnf>
    <type>{{vnf.type}}</type>
    {% for fpath in vnf.fpaths %}
    <vm>
      <fpath-id>{{fpath}}</fpath-id>
    </vm>
    {% endfor %}
  </vnf>
  {% endfor %}
</vnfs>
""")

FPATH = Template("""
<network>
  <colony>
    {% for context in contexts %}
    <network-context>
      <name>{{context.name}}</name>
      {% for iface in context.interfaces %}
      <interface>
        {% for fpath in iface.fpaths %}
        <port>
          <fastpath-instance>{{fpath}}</fastpath-instance>
        </port>
        {% endfor %}
      </interface>
      {% endfor %}
    </network-context>
   {% endfor %}
  </colony>
</network>
""")

class TestConnectionBuilder(unittest.TestCase):


    def test_convert(self):
        meta = {
            'vnfs' : [{
                'type' : 't1',
                'fpaths' : ['a', 'b', 'c']
            },{
                'type' : 't2',
                'fpaths' : ['d', 'e', 'f'],

            }],
            'contexts' : [{
                'name' : 'c1',
                'interfaces' : [
                    { 'fpaths' : ['a', 'b'] },
                    { 'fpaths' : ['c', 'd'] }
                ]},{
                'name' : 'c2',
                'interfaces' : [
                    { 'fpaths' : ['e'] },
                    { 'fpaths' : ['f'] }
                ]
            }]
        }

        bldr = vnf.ConnectionBuilder()
        vnf_xml = XML(VNF.render(meta))
        #print(tostring(vnf, pretty_print=True))
        fpath = XML(FPATH.render(meta))
        #print(tostring(fpath, pretty_print=True))
        connectors = bldr.source_connectors(vnf_xml, fpath)
        #print(connectors)
        self.assertEqual(2, len(connectors))


    def test_implied_slowpath_connectors(self):
        meta = {
            'vnfs' : [{
                'type' : 'ltemmesim',
                'fpaths' : []
            },{
                'type' : 'ltegwsim',
                'fpaths' : [],

            }],
        }
        bldr = vnf.ConnectionBuilder()
        vnf_xml = XML(VNF.render(meta))
        bldr.implied_slowpath_connectors(vnf_xml)
        #print(tostring(vnf_xml, pretty_print=True))
        connectors = vnf_xml.xpath('//connector')
        self.assertEqual(2, len(connectors))
        destinations = vnf_xml.xpath('//connector/destination')
        self.assertEqual(1, len(destinations))

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
