
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import argparse
import logging
import os
import io
import sys
import gi
import abc
from gi.repository import RwYang
import xmlrunner

class Visitor(object):
    __metaclass__ = abc.ABCMeta

    @abc.abstractmethod
    def visit_leaf(self, node_type, node):
        return

    @abc.abstractmethod
    def enter_collection(self, node_type, node):
        return

    @abc.abstractmethod
    def exit_collection(self, node_type, node):
        return

class Schema(object):
    def __init__(self, keep_empty_collections=True):
        self.root = {'properties':[]}
        self.stack = [self.root]
        self.keep_empty_collections = keep_empty_collections

    def visit_leaf(self, node_type, node):
        m = self.meta(node_type, node)
        m['is_config'] = (node.is_config() == 1)

    def enter_collection(self, node_type, node):
        m = self.meta(node_type, node)
        m['properties'] = []
        if node.has_keys() == 1 and node_type == 'list':
            # can have compound keys, but not supported ATM
            m['key'] = node.get_first_key().get_key_node().get_name()
        self.stack.append(m)

    def exit_collection(self, node_type, node):
        c = self.stack.pop()

        if self.keep_empty_collections is False:
            if len(c['properties']) == 0:
                self.stack[-1]['properties'].remove(c)

    def meta(self, node_type, node):
        meta = {
            'name' : node.get_name(),
            'type' : node_type,
            'prefix' : node.get_prefix(),
            'description' : node.get_description()
        }
        self.stack[-1]['properties'].append(meta)
        return meta

# Walk only config nodes.
class ConfigOnly(object):
    def valid_node(self, node):
        return node.is_leafy() == 0 or node.is_config() != 0

# Walk nodes that pass all of a given filter set
class Filters(object):
    def __init__(self, filters):
        self.filters = filters

    def valid_node(self, node):
        for f in self.filters:
            if f.valid_node(node) == False:
                return False


Visitor.register(Schema)

class Browser(object):

    def __init__(self, base):
        # presume this sets up what meta is loaded
        yang_model = RwYang.Model.create_libncx()
        # Waiting for fix to RIFT-5662 
        yang_model.load_module('rw-base')
        yang_model.load_module('rw-fpath')
        yang_model.load_module('rw-ifmgr-data')
        self.root = yang_model.get_root_node()

    def walk(self, visitor, filter=None):
        self.walk_node(self.root, visitor, filter)

    def walk_node(self, node, visitor, filter):
        if filter is not None and filter.valid_node(node) is False:
            return

        if node.is_leafy():
            visitor.visit_leaf('leaf', node)
        else:
            node_type = None
            if node.is_listy():
                node_type = 'list'
            else:
                node_type = 'container'
            visitor.enter_collection(node_type, node)
            child = node.get_first_child()
            while child is not None:
                self.walk_node(child, visitor, filter)
                child = child.get_next_sibling()
            visitor.exit_collection(node_type, node)
