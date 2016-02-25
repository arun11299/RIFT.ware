
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import lxml.etree
from rwmgmtapi.app import app, cache
import rwmgmtapi.rwxml
import rwmgmtapi.xml2json
import rwmgmtapi.fpath

def xml_request(netconf, request):
    return lxml.etree.XML(netconf.get(request))

# Generic reader that find one node and strips namespace.  Essentially model from NETCONF
# is good enough, no post processing required
def xml_strip(xml, xpath):
    new_root = xml.xpath(xpath, namespaces=rwmgmtapi.rwxml.NSMAP)
    if len(new_root) == 0:
        raise ("xpath %s not found in doc" % xpath)
    return rwmgmtapi.rwxml.remove_ns(new_root[0])

@app.route('/fpath/')
@cache.cached()
def fpath():
    xml, _ = rwmgmtapi.fpath.Service().fpath()
    return rwmgmtapi.rwxml.xml_content(lxml.etree.tostring(xml), rwmgmtapi.rwxml.CONFIG_SCHEMA)

FPATH_XML = rwmgmtapi.xml2json.XmlDecoderStrategy()
FPATH_XML.is_list = ['name']

class FPathXmlBuilder(rwmgmtapi.rwxml.Builder):

    def colony(self, fpath_colony_id):
        self.base('colony')
        self.e.append(rwmgmtapi.rwxml.text_elem(rwmgmtapi.rwxml.BASE + 'name', fpath_colony_id))
        return self

    # trafgen start/stop commands have colony in fpath namespace
    def fpath_colony(self, fpath_colony_id):
        self.fpath('colony')
        self.e.append(rwmgmtapi.rwxml.text_elem(rwmgmtapi.rwxml.FPATH + 'name', fpath_colony_id))
        return self

    # diameter start/stop commands have colony in appmgr namespace
    def appmgr_colony(self, fpath_colony_id):
        self.appmgr('colony')
        self.e.append(rwmgmtapi.rwxml.text_elem(rwmgmtapi.rwxml.APPMGR + 'name', fpath_colony_id))
        return self

    # if port is not empty
    #  <port>name</port>
    def optional_port(self, port_name):
        if port_name is not None and len(port_name) > 0:
            self.e.append(rwmgmtapi.rwxml.text_elem(rwmgmtapi.rwxml.FPATH + 'port', port_name))
        return self

    # <port><name>name</name><port>
    def port(self, port_name):
        self.fpath('port')
        self.e.append(rwmgmtapi.rwxml.text_elem(rwmgmtapi.rwxml.FPATH + 'name', port_name))
        return self
