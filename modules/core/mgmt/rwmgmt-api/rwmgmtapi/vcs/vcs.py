
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import lxml.etree
import rwmgmtapi.rwxml
import rwmgmtapi.confd
import rwmgmtapi.fpath

# vcs
#  colonies
#    vms
#      processes
#      tasklets
#    clusters
#       vms
#		   processes
#            tasklets
#
class Driver(object):

    def get_base(self):
        cr = rwmgmtapi.confd.ConfDRequest()
        cr.accept(rwmgmtapi.confd.ContentType.YANG_DATA_XML)
        base_config = cr.flask_require('/operational/tasklet?deep')
        xml = lxml.etree.XML(base_config)
        return xml

class Service(object):
    def __init__(self):
        self.driver = Driver()
        self.fpath = rwmgmtapi.fpath.Service()

    def vcs(self):
        flat = self.driver.get_base()
        tree = TreeBuilder().build(flat)
        self.fpath_xml = FpathBuilder().append_fpath(tree, self.fpath)
        return tree

# Takes tasklet info as flat list from API and re-organizes list into a heirarchy
# by reading the children elements.  This does not remove any information from
# DOM, only reorganizes it.
class TreeBuilder(object):

    def build(self, tasklet_info):
        sector = lxml.etree.Element("collection")
        collection_info = lxml.etree.Element('collection_info')
        collection_info.append(rwmgmtapi.rwxml.text_elem('collection-type', 'rwsector'))
        sector.append(collection_info)
        sector.append(rwmgmtapi.rwxml.text_elem('component_type', 'RWCOLLECTION'))
        info_elems = tasklet_info.xpath('//rw-base:component_info', namespaces=rwmgmtapi.confd.Ns.NSMAP)
        tree = self.build_tree(info_elems)
        self.walk_tree(tree, sector, self.add_collection)
        return sector

    def build_tree(self, info_elems):
        tag_names = {
            'RWCOLLECTION' : 'collection',
            'RWPROC' : 'process',
            'PROC' : 'process',
            'RWTASKLET' : 'tasklet',
            'RWVM' : 'vm'
        }
        tree = {}
        for info_elem in info_elems:
            type = info_elem.findtext(rwmgmtapi.rwxml.BASE + 'component_type')
            tag_name = tag_names[type]
            elem = rwmgmtapi.rwxml.remove_ns(rwmgmtapi.rwxml.clone_elem(tag_name, info_elem))
            name = elem.findtext('instance_name')
            children = []
            for name_elem in elem.iter('rwcomponent_children'):
                children.append(name_elem.text)
            tree[name] = { 'elem' : elem, 'parent' : None, 'children' : children }

        for name, item in tree.iteritems():
            for child_name in item['children']:
                tree[child_name]['parent'] = item

        return tree

    def walk_tree(self, tree, sector, visitor):
        for name, item in tree.iteritems():
            if item['parent'] == None:
                self.walk_item(tree, sector, item, 1, visitor)

    def walk_item(self, tree, parent, item, level, visitor):
        visitor(parent, item['elem'], level)
        for child_name in item['children']:
            self.walk_item(tree, item['elem'], tree[child_name], level + 1, visitor)

    def add_collection(self, parent, item, level):
        if item.tag == 'tasklet':
            if item.findtext('component_name') == 'RW.Fpath':
                port = lxml.etree.Element('fpath-id')
                port.text = item.findtext('instance_id')
                vm = parent.getparent()
                vm.append(port)

        parent.append(item)


# Associate Fastpath configuration into the VMs listed in VCS tree.  This
# relies on the Fastpath DOM and the VCS DOM each having fields in that correlate
# data.
class FpathBuilder(object):

    def append_fpath(self, vcs, fpath):
        fpath_config, port_builder = fpath.fpath()

        # RIFT-5746 - We cache results because VNF will need a copy of xml for
        # building connectors
        fpath_config_copy = lxml.etree.XML(lxml.etree.tostring(fpath_config))

        self.source_connectors(vcs, fpath_config_copy)
        port_builder.append_fabric_ports(vcs)
        return fpath_config

    def source_connectors(self, vcs, fpath_config):
        fpath_elems = vcs.xpath('//vm/fpath-id')
        for fpath_elem in fpath_elems:
            fpath_id = fpath_elem.text
            fpath_matches = fpath_config.xpath('//fastpath-instance[text()=\'%s\']' % fpath_id)
            for fpath_match in fpath_matches:
                port = fpath_match.getparent()
                fpath_elem.getparent().append(port)
