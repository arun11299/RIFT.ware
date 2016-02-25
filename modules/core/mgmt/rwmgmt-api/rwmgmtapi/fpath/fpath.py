
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import lxml.etree
import rwmgmtapi.confd
import rwmgmtapi.rwxml


class Driver(object):

    def get_base_config(self):
        cr = rwmgmtapi.confd.ConfDRequest()
        cr.accept(rwmgmtapi.confd.ContentType.YANG_COLLECTION_XML)
        # cannot get port-state in one-shot by adding port-state to select, but be in different section somehow
        base_config = cr.flask_require('/running/colony?select=name;port;network-context(*)')
        return lxml.etree.XML(base_config)

    def get_port_meta(self):
        cr = rwmgmtapi.confd.ConfDRequest()
        cr.accept(rwmgmtapi.confd.ContentType.YANG_DATA_XML)
        q = rwmgmtapi.confd.QueryBuilder()
        q.query('/colony')
        q.select('network-context/interface/name')
        q.select('network-context/interface/bind/port')
        cr.set_payload(q.str())
        port_meta = cr.flask_require('/query/operational')
        return lxml.etree.XML(port_meta)

    def colony_xml(self, colony):
        cr = rwmgmtapi.confd.ConfDRequest()
        cr.accept(rwmgmtapi.confd.ContentType.YANG_COLLECTION_XML)
        state = cr.flask_require('/operational/colony/%s/port-state?select=portname;info(*)' % colony)
        return lxml.etree.XML(state)

class Service(object):
    def __init__(self):
        self.driver = Driver()

    def fpath(self):
        config = self.driver.get_base_config()
        b1 = BaseConfigBuilder()
        config2 = b1.append_interface_details(config)
        b2 = PortStatsBuilder(self.driver)
        b2.append_stats(config, config2)
        return config2, b2

class BaseConfigBuilder:

    def append_interface_details(self, base_config):
        root = lxml.etree.Element('network')
        port_xpath = lxml.etree.XPath('rw-base:colony/rw-portconfig:port', namespaces=rwmgmtapi.confd.Ns.NSMAP)
        port_map = rwmgmtapi.rwxml.build_index(base_config, port_xpath, rwmgmtapi.rwxml.get_fpath_name)
        context_elems = base_config.xpath('rw-base:colony/rw-base:network-context', namespaces=rwmgmtapi.confd.Ns.NSMAP)
        for context_elem in context_elems:
            context = rwmgmtapi.rwxml.remove_ns(rwmgmtapi.rwxml.clone_elem('network-context', context_elem))
            context_name = context.findtext('name')
            colony_name = rwmgmtapi.rwxml.get_base_name(context_elem.getparent())
            root.append(context)

            iface_elems = context.xpath('interface')
            for _, iface_elem in enumerate(iface_elems):
                iface_elem.append(rwmgmtapi.rwxml.text_elem('colonyId', colony_name))
                iface_elem.append(rwmgmtapi.rwxml.text_elem('context', context_name))

            bind_elems = context.xpath('interface/bind/port')
            for _, bind_elem in enumerate(bind_elems):
                if not bind_elem.text in port_map:
                    raise KeyError('Could not find port %s in colony/port list' %  bind_elem.text)
                port_elem =  port_map[bind_elem.text]
                port = rwmgmtapi.rwxml.remove_ns(rwmgmtapi.rwxml.clone_elem('port', port_elem))
                port_name = port.findtext('name')
                id = '%s|%s' % (colony_name, port_name)
                port.set('id', id)
                port.append(rwmgmtapi.rwxml.text_elem('colonyId', colony_name))
                iface = bind_elem.getparent().getparent()
                iface.append(port)

        return root

class PortStatsBuilder:
    def __init__(self, fpath_driver):
        self.driver = fpath_driver
        self.fabric_ports = []

    # Call append_fabric_ports after
    def append_stats(self, root_config, base_config):
        colony_names = root_config.xpath('rw-base:colony/rw-base:name', namespaces=rwmgmtapi.confd.Ns.NSMAP)
        for colony_name_elem in colony_names:
            colony_name = colony_name_elem.text
            port_state = self.driver.colony_xml(colony_name)
            self.append_state(base_config, port_state, colony_name)

        self.aggregate_port_stats(base_config)

    def aggregate_port_stats(self, root):
        iface_elems = root.xpath('//interface')
        for iface_elem in iface_elems:
            port_elems = iface_elem.findall('port')
            speed = 0
            state = 'up'
            for port_elem in port_elems:
                # if speed is null, fpath may have crashed
                speed += int(port_elem.findtext('speed'))
                port_state = port_elem.findtext('state')
                if (state != 'up'):
                    state = port_state
            iface_elem.append(rwmgmtapi.rwxml.text_elem('speed', str(speed)))
            iface_elem.append(rwmgmtapi.rwxml.text_elem('state', state))


    def append_state(self, root, state, colony_name):
        port_state_elems = state.xpath('rw-ifmgr-data:port-state/rw-ifmgr-data:info',
                                       namespaces=rwmgmtapi.confd.Ns.NSMAP)
        for port_state_elem in port_state_elems:
            port_name = port_state_elem.getparent().findtext(rwmgmtapi.rwxml.IFMGR + 'portname')
            # port_name = rwmgmtapi.rwxml.get_fpath_name(port_state_elem)
            id = '%s|%s' % (colony_name, port_name)
            port = root.xpath('//port[@id=\'%s\']' % id)
            if len(port) > 0:
                for child_elem in port_state_elem:
                    port[0].append(rwmgmtapi.rwxml.remove_ns(child_elem))
            else:
                fabric_port = rwmgmtapi.rwxml.remove_ns(port_state_elem)
                fabric_port.append(rwmgmtapi.rwxml.text_elem("colonyId", colony_name))
                fabric_port.append(rwmgmtapi.rwxml.text_elem("name", port_name))
                fabric_port.tag = 'port'
                self.fabric_ports.append(fabric_port)

    def append_fabric_ports(self, xml):
        fpath_elems = xml.xpath('//vm/fpath-id')
        for fpath_elem in fpath_elems:
            fabric = lxml.etree.Element('fabric')
            colony_elem = lxml.etree.Element('colonyId')
            fabric.append(colony_elem)
            fpath_elem.getparent().append(fabric)
            fpath_id = fpath_elem.text
            for port in self.fabric_ports:
                port_fpath_id = port.findtext('fastpath-instance')
                if fpath_id == port_fpath_id:
                    fabric.append(port)
                colony_elem.text = port.findtext('colonyId')
