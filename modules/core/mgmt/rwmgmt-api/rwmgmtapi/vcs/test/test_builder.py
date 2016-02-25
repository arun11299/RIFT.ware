
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import unittest
import lxml.etree
from rwmgmtapi.vcs import vcs
from jinja2 import Template
import rwmgmtapi

TASKLET_INFO = Template("""
<vcs xmlns="http://riftio.com/ns/riftware-1.0/rw-base"
    xmlns:y="http://tail-f.com/ns/rest"
    xmlns:rw-base="http://riftio.com/ns/riftware-1.0/rw-base">
  <info>
    <components>
      {% for c in components %}
      <component_info>
        <instance_name>{{c.name}}</instance_name>
        <component_type>{{c.type}}</component_type>
        {% for child in c.children %}
          <rwcomponent_children>{{child}}</rwcomponent_children>
        {% endfor %}
      </component_info>
      {% endfor %}
    </components>
  </info>
</vcs>
""")

class TestRift(unittest.TestCase):
    meta = {
        'components' : [
            {'name' : 'a', 'type': 'RWVM'},
            {'name' : 'b', 'type': 'RWVM'},
            {'name' : 'c', 'type': 'RWVM'},
        ]
    }

    def testIssue(self):
        tasklet_info = lxml.etree.XML(TASKLET_INFO.render(self.meta))
        #print(lxml.etree.tostring(tasklet_info))
        nodes = tasklet_info.xpath('/rw-base:vcs/rw-base:info/rw-base:components/rw-base:component_info',
            namespaces=rwmgmtapi.confd.Ns.NSMAP)
        self.assertEqual(3, len(nodes))
        print(lxml.etree.tostring(tasklet_info))


class TestTreeBuilder(unittest.TestCase):

    meta = {
        'components' : [
            {'name' : 'a', 'type': 'RWCOLLECTION', 'children' : ['b'] },
            {'name' : 'b', 'type': 'RWCOLLECTION', 'children' : ['c']},
            {'name' : 'c', 'type': 'RWVM'}
        ]
    }

    def testBuild(self):
        tasklet_info = lxml.etree.XML(TASKLET_INFO.render(self.meta))
        #print(lxml.etree.tostring(tasklet_info))
        bldr = vcs.TreeBuilder()
        actual = bldr.build(tasklet_info)
        print(lxml.etree.tostring(actual, pretty_print=True))
        clusters = actual.xpath('/collection/collection/collection')
        self.assertEqual(1, len(clusters))
        vms = actual.xpath('/collection/collection/collection/vm')
        self.assertEqual(1, len(vms))

VCS_INFO = Template("""
<sector>
  <colony>
    <cluster>
      {% for v in vms %}
      <vm>
        <fpath-id>{{v.fpath}}</fpath-id>
      </vm>
      {% endfor %}
    </cluster>
  </colony>
</sector>
""")

FPATH_INFO = Template("""
<fpath>
    {% for p in ports %}
      <port>
        <fastpath-instance>{{p.fpath}}</fastpath-instance>
      </port>
    {% endfor %}
</fpath>
""")

class TestFpathBuilder(unittest.TestCase):

    fpath = [
        {'fpath' : 'a'}
    ]

    meta = {
        'vms' : fpath,
        'ports' : fpath
    }

    def testAppend(self):
        vcs_info = lxml.etree.XML(VCS_INFO.render(self.meta))
        #print(lxml.etree.tostring(vcs_info))
        fpath_info = lxml.etree.XML(FPATH_INFO.render(self.meta))
        #print(lxml.etree.tostring(fpath_info))
        bldr = vcs.FpathBuilder()
        bldr.source_connectors(vcs_info, fpath_info)
        #print(lxml.etree.tostring(vcs_info, pretty_print=True))
        a_port = vcs_info.xpath('//vm/port/fastpath-instance')
        self.assertEqual(1, len(a_port))
        self.assertEqual('a', a_port[0].text)


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
