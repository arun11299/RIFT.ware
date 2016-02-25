
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import re
import copy
import lxml.etree
import rwmgmtapi.rwxml
import rwmgmtapi.fpath
import rwmgmtapi.vcs


class Service:
    def __init__(self):
        self.fpath = rwmgmtapi.fpath.Service()
        self.vcs = rwmgmtapi.vcs.Service()

    def vnfs(self):
        vcs_config = self.vcs.vcs()
        vnfs = VcsConverter().convert(vcs_config)
        fpath = self.vcs.fpath_xml
        cb = ConnectionBuilder()
        connectors = cb.source_connectors(vnfs, fpath)
        cb.destination_connectors(vnfs, connectors)

        # Comment out as we need to re-evaulating if this is the coreect strategy
        # cb.implied_slowpath_connectors(vnfs)

        return vnfs

# There is no VNF available yet so we derive this from VCS and
# a particular naming convention defined in apifill code
class VcsConverter:
    def __init__(self):
        self.apifill = {
            'trafgen': re.compile(r".*[Tt]raf[Gg]en.*"),
            'trafsink': re.compile(r".*[Tt]raf[Ss]ink.*"),
            'trafsimclient': re.compile(r".*([Tt]raf[Ss]im[Cc]lient|seagull_client).*"),
            'trafsimserver': re.compile(r".*([Tt]raf[Ss]im[Ss]erver|seagull_server).*"),
            'ltemmesim': re.compile(r".*ltemmesim.*"),
            'ltegwsim': re.compile(r".*ltegwsim.*"),
            'ltecombinedsim': re.compile(r".*ltecombinedsim.*"),
            'loadbal': re.compile(r".*[Ll]oad[Bb]al.*"),
            'slbalancer': re.compile(r".*slbalancer.*"),
            'iot_server': re.compile(r".*iot_server.*"),
            'iot_army': re.compile(r".*iot_army.*"),
            'premise_gw': re.compile(r".*premise_gw.*"),
            'cag': re.compile(r".*cag.*")
        }
        # RIFT-5209, RIFT-5030 - ignore master cluster vms as VNFs, they (misleadingly)
        # have the suffix 'leader', not to be confused with actual cluster leaders which
        # is separate.
        self.is_leader = re.compile(r".*leader.*")

    def convert(self, vcs):
        root = lxml.etree.Element('vnfs')
        groups = {}
        vm_elems = vcs.xpath('//vm')
        vnfs = {}
        for vm_elem in vm_elems:
            name = vm_elem.findtext('component_name')
            if self.is_leader.match(name):
                continue
            for tag, apifill in self.apifill.iteritems():
                if apifill.match(name):
                    if tag == 'ltecombinedsim':
                        vnfmme = self.add_vnf('ltemmesim', vm_elem, vnfs, root)

                        # copy xml otherwise updating DOM can get wacky w/updates
                        # going in multiple place
                        vm_elem_copy = copy.deepcopy(vm_elem)
                        vnfgw = self.add_vnf('ltegwsim', vm_elem_copy, vnfs, root)
                    else:
                        vnf = self.add_vnf(tag, vm_elem, vnfs, root)
        return root

    def add_vnf(self, tag, elem, vnfs, root):
        vnf = None
        if tag in vnfs:
            vnf = vnfs[tag]
        else :
            vnf = lxml.etree.Element('vnf')
            vnf.append(rwmgmtapi.rwxml.text_elem('name', tag))
            vnf.append(rwmgmtapi.rwxml.text_elem('type', tag))
            root.append(vnf)
            vnfs[tag] = vnf
        vnf.append(elem)

# There is no programatic way to determine what ports are connected way so rift has
# adopted a convention that relying on network context naming
class ConnectionBuilder(object):

    def __init__(self):
        self.slowpath_id = 0

    def implied_slowpath_connectors(self, vnfs):
        mme_elems = vnfs.xpath('//vnf[type=\'ltemmesim\']')
        gw_elems = vnfs.xpath('//vnf[type=\'ltegwsim\']')
        for gw_elem in gw_elems:
            gw_connector = lxml.etree.Element('connector')
            gw_connector_id = self.next_slowpath_id()
            gw_connector.set('id', gw_connector_id)
            gw_elem.append(gw_connector)
            for mme_elem in mme_elems:
                mme_connector = lxml.etree.Element('connector')
                mme_connector_id = self.next_slowpath_id()
                mme_connector.set('id', mme_connector_id)
                dest = lxml.etree.Element('destination')
                dest.text = gw_connector_id
                mme_connector.append(dest)
                mme_elem.append(mme_connector)

    def next_slowpath_id(self):
        id = 'slowpath|' + str(self.slowpath_id)
        self.slowpath_id += 1
        return id

    def source_connectors(self, vnfs, fpath):
        connector_map = {}
        fpath_elems = vnfs.xpath('//vm/fpath-id')
        for fpath_elem in fpath_elems:
            fpath_id = fpath_elem.text
            fpath_matches = fpath.xpath('//fastpath-instance[text()=\'%s\']' % fpath_id)
            for fpath_match in fpath_matches:
                iface = fpath_match.getparent().getparent()

                context = iface.getparent().findtext('name')
                network_id = re.sub(r'\d*$', '', context)

                vnf = fpath_elem.getparent().getparent()
                vnf_type = vnf.findtext('type')

                connector_id = vnf_type + '|' + network_id
                if connector_id in connector_map:
                    connector = connector_map[connector_id]
                else:
                    connector = lxml.etree.Element('connector')
                    connector.set('id', connector_id)
                    connector.set('networkId', network_id)
                    connector.set('context', context)
                    connector_map[connector_id] = connector
                    vnf.append(connector)

                connector.append(iface)

        return connector_map.values()

    def destination_connectors(self, vnf, connectors):
        for a in connectors:
            a_type = a.getparent().findtext('type')
            # Sinks never connect anywhere
            if self.is_sink_vnf(a_type):
                continue
            a_network = a.get('networkId')
            for b in connectors:
                if a != b:
                    b_network = b.get('networkId')
                    b_type = b.getparent().findtext('type')

                    # 2 sources never connect to each other
                    if self.is_source_vnf(a_type) and self.is_source_vnf(b_type):
                       continue

                    if self.is_mediator_vnf(a_type) and self.is_source_vnf(b_type):
                        continue

                    if a_network == b_network:
                        dest = lxml.etree.Element('destination')
                        dest.text = b.get('id')
                        a.append(dest)

    def is_mediator_vnf(self, type):
        return (type == 'loadbal' or type == 'slbalancer' or type ==
        'premise_gw' or type == 'cag')

    def is_source_vnf(self, type):
        return (type == 'trafgen' or type == 'trafsimclient' or type ==
            'ltemmesim' or type == 'iot_army')

    def is_sink_vnf(self, type):
        return (type == 'trafsink' or type == 'trafsimserver' or type ==
            'ltegwsim' or type == 'iot_server')

    def network_id(self, iface):
        ip = iface.xpath('ip/address/text()')[0]
        id, _, _ = ip.rpartition('.')
        return id
