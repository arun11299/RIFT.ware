
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import lxml.etree
from lxml import objectify
import copy
import json
import StringIO
import flask
import rwmgmtapi.xml2json
import string

MIME_TYPES = ['application/xml', 'application/json', 'application/text']
FPATH_NS = "http://riftio.com/ns/riftware-1.0/rw-portconfig"
BASE_NS = "http://riftio.com/ns/riftware-1.0/rw-base"
APPMGR_NS = "http://riftio.com/ns/riftware-1.0/rwappmgr"
IFMGR_NS = "http://riftio.com/ns/riftware-1.0/rw-ifmgr-data"
FPATH = "{%s}" % FPATH_NS
BASE = "{%s}" % BASE_NS
APPMGR = "{%s}" % APPMGR_NS
IFMGR = "{%s}" % IFMGR_NS
FPATH_PREFIX = 'rw-portconfig'
APPMGR_PREFIX = 'rwappmgr'
BASE_PREFIX = 'rw-base'
IFMGR_PREFIX = 'rw-ifmgr-data'
NSMAP = {
    BASE_PREFIX : BASE_NS,
    FPATH_PREFIX : FPATH_NS,
    APPMGR_PREFIX : APPMGR_NS,
    IFMGR_PREFIX : IFMGR_NS
}

CONFIG_SCHEMA = rwmgmtapi.xml2json.XmlDecoderStrategy()
CONFIG_SCHEMA.is_list = [
    'rw-base:node',
    'rw-base:interface',
    'rw-base:resources',
    'rw-base:vm',
    'rw-fpath:port',
    'rw-fpath:destination-nat',
    'RwvcsRwcomponentInfo',
    'rwcomponent_children',
    'response',
    'rw-base:network-context',
    'network-context',
    'rw-base:colony',
    'component_info',
    'collection',
    'vm',
    'process',
    'tasklet',
    'port',
    'vnf',
    'fpath-id',
    'connector',
    'interface',
    'destination',
    'worker',
    'fastpath',
    'timer',
    'trafsim-service',
    'traffic',
    'command',
    'commands',
    'individual'
]
CONFIG_SCHEMA.has_attributes = [
    'response',
    'vnf',
    'connector'
]

# Keep CR, LF and Tabs
CTL_CHARS = ''.join(chr(i) for i in range(32))
RPL_CHAR = '         \x09\x0a  \x0d                  '
#           012345678...9...abc...def0123456789abcdef
CLEAN_CONTROL = string.maketrans(CTL_CHARS, RPL_CHAR)

def clone_elem(name, elem):
    clone = lxml.etree.Element(name)
    for attr, value in elem.items():
        clone.set(name, value)
    for child in elem:
        clone.append(copy.deepcopy(child))
    return clone

def merge_elem(a, b):
    for attr, value in b.items():
        a.set(attr, value)
    for child in b:
        a.append(copy.deepcopy(child))
    return a

def remove_ns(root):
    for elem in root.getiterator():
        i = elem.tag.find('}')
        if i >= 0:
            elem.tag = elem.tag[i+1:]
    objectify.deannotate(root, cleanup_namespaces=True)
    return root

def text_elem(tag, text):
    e = lxml.etree.Element(tag)
    e.text = text
    return e

def get_fpath_name(elem):
    return elem.findtext('{%s}name' % FPATH_NS)

def get_base_name(elem):
    return elem.findtext('{%s}name' % BASE_NS)

def build_index(root, xpath_function, key_func):
    map = {}
    elems = xpath_function(root)
    for elem in elems:
        key = key_func(elem)
        map[key] = elem
    return map

class Builder:
    def __init__(self, rootTag='root'):
        self.xml = self.e = lxml.etree.Element(rootTag)
        self.firsts = {}

    def str(self):
        return lxml.etree.tostring(self.xml)

    def text(self, text):
        self.e.text = text
        return self

    # if you add a tag but do not want to append to that tag
    # Example
    #  <a>
    #    <b>x</b>
    #    <c>
    # Is
    #   ...tag(b).text('x').up().tag(c)...
    #
    def up(self):
        self.e = self.e.getparent()
        return self

    def fpath(self, tag):
        return self.nstag(FPATH_PREFIX, FPATH, tag)

    def base(self, tag):
        return self.nstag(BASE_PREFIX, BASE, tag)

    def appmgr(self, tag):
        return self.nstag(APPMGR_PREFIX, APPMGR, tag)

    # ensured ns reference is only in file once and only as needed
    def nstag(self, ns_prefix, ns, tag):
        e = None
        if ns in self.firsts:
            e = lxml.etree.Element(ns + tag)
        else:
            e = lxml.etree.Element(ns + tag, nsmap={ns_prefix : NSMAP[ns_prefix]})
            self.firsts[ns] = True
        self.e = self.append(e)
        return self

    def append(self, e):
        self.e.append(e)
        return e

    def map(self, meta, ns, parent=None):
        if parent is None:
            parent = self.e
        if isinstance(meta, dict):
            for tag, value in meta.iteritems():
                child = self.append(lxml.etree.Element(ns + tag))
                parent.append(child)
                self.map(value, ns, child)
        else:
            parent.text = str(meta)
        return self

def xml_to_json(xml, strategy=None):
    s = StringIO.StringIO(xml)
    decoder = rwmgmtapi.xml2json.XmlDecoder(strategy)
    data = decoder.decode(s)
    return json.dumps(data)

def xml_content(xml, strategy=None):
    best = flask.request.accept_mimetypes.best_match(MIME_TYPES)
    if best == 'application/json':
        return xml_to_json(xml, strategy)
    return xml

def json_content(json):
	# TODO
	return json


def clean(s):
    # RIFT-4087
    return s.translate(CLEAN_CONTROL)

