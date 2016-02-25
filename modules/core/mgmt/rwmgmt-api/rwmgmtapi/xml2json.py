
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import xml.sax
import xml.sax.handler
import collections

CONTENT_ID = '#content'

class XmlDecoderStrategy:
    def __init__(self):
        self.is_list = []
        self.has_attributes = []

    def element_is_list(self, name, path):
        return len(path) > 1 and name in self.is_list

    def element_has_attributes(self, name, path):
        return name in self.has_attributes

    def element_ignore_whitespace(self):
        return True

class XmlDecoder(xml.sax.handler.ContentHandler):
    def __init__(self, strategy=None):
        self.path = []
        self.parents = []
        self.strategy = strategy or XmlDecoderStrategy()

    def decode(self, xml_stream):
        parser = xml.sax.make_parser()
        parser.setContentHandler(self)
        parser.parse(xml_stream)
        return self.root

    def startDocument(self):
        self.path.append('')
        self.root = collections.OrderedDict()
        self.parents.append(self.root)

    def startElement(self, name, attrs):
        self.pushParent(name)        
        if self.strategy.element_has_attributes(name, self.path):
            for name in attrs.getNames():
                self.pushAttr('@' + name, attrs.getValue(name))

    def pushParent(self, name):
        self.path.append(name)
        self.is_list = self.strategy.element_is_list(name, self.path)
        parent = self.parents[-1]
        child = collections.OrderedDict()
        if self.is_list:
            if name in parent:
                parent[name].append(child)
            else:
                parent[name] = [ child ]
        else:
            parent[name] = child

        self.parents.append(child)

    def pushAttr(self, name, value):
        self.parents[-1][name] = value

    def pushValue(self, value):
        insertion_point = self.parents[-2][self.path[-1]]
        if isinstance(insertion_point, list):
            if len(insertion_point[-1]) == 0:
                insertion_point[-1] = value
            else:
                insertion_point[-1] += "\n" + value
        else:
            if len(self.parents[-2][self.path[-1]]) == 0:
                self.parents[-2][self.path[-1]] = value
            else:
                self.parents[-2][self.path[-1]] += "\n" + value

    def characters(self, content):
        if self.strategy.element_ignore_whitespace():
            if not content or len(content.strip()) == 0:
                return
        if self.path[-1] == '@' or self.strategy.element_has_attributes(self.path[-1], self.path):
            self.pushAttr(CONTENT_ID, content)
        else:
            self.pushValue(content)

    def endElement(self, name):
        path = self.path.pop()
        self.parents.pop()
        if path == '@':
            self.path.pop()
            self.parents.pop()
