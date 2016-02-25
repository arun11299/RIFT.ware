# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Max Beckett
# Creation Date: 7/10/2015
# 

import json    
import lxml.etree

import tornado.escape

from ..util import (
    find_child_by_name,
    find_child_by_path,
    find_target_type,
    split_url,
    TargetType,
)

from gi.repository import RwYang
from rift.rwlib.schema import collect_children
from rift.rwlib.util import iterate_with_lookahead
from rift.rwlib.xml import (
    collect_siblings,
    get_xml_element_name,
)

def _split_namespace(namespace):
    split_namespace = namespace.split("/")

    if len(split_namespace) == 1:
        split_namespace = namespace.split(":")            

    prefix = '%s:' % split_namespace[-1]
    return prefix

def _split_tag_and_update_prefixes(element, prefixes):
    ''' Strip out unqualified name, and it's associated prefix, and maintain prefixes stack'''
    pieces = element.tag.split("}")

    try:
        name = pieces[-1]
        namespace = pieces[0]
        prefix = _split_namespace(namespace)

        if prefix.startswith("{"):
            # prune leading "{"
            prefix = prefix[1:] 

        if not prefixes:
            prefixes.append(prefix)
        elif prefixes[-1] == prefix:
            prefix = ""
        else:
            prefixes.append(prefix)
    except IndexError as e:
        name = pieces[0]
        prefix = ""

    return name, prefix, prefixes


class XmlToJsonTranslator(object):
    ''' Converts the results of a NETCONF query into JSON.'''

    def __init__(self, schema):
        self._schema = schema

    def convert(self, is_collection, url, xpath, xml_string):
        ''' Entry for XML -> JSON conversion
        
        is_collection -- True if the results is in the collection+json format
        xpath -- the xpath corresponding to the query results being translated
        xml_string -- the XML results to translate
        '''

        url_pieces, *_ = split_url(url)

        schema_root = find_child_by_path(self._schema, url_pieces)
        target_type = find_target_type(self._schema, url_pieces)
        xml_root = lxml.etree.fromstring(xml_string)

        elements_to_translate = xml_root.xpath(xpath)

        json = list()
        prefixes = list()

        is_list = schema_root.is_listy() and target_type == TargetType.List
        is_leaf = schema_root.is_leafy()

        json.append("{")
        if is_collection or is_list:

            pieces = elements_to_translate[0].tag.split("}")
            try:
                target_name = pieces[1]
                target_prefix = _split_namespace(schema_root.get_ns())
                prefixes.append(target_prefix)
            except IndexError:
                target_name = pieces[0]
                target_prefix = ""

                
        if is_collection:
            json.append('"collection":{')
            json.append('"%s%s":[' % (target_prefix, target_name))

        elif is_list:
            json.append('"%s%s":[' % (target_prefix, target_name))

        for element, is_last in iterate_with_lookahead(elements_to_translate):

            name, prefix, prefixes = _split_tag_and_update_prefixes(element, prefixes)

            schema_node = find_child_by_name(self._schema, name)

            if is_collection or is_list:
                json.append('{')
            elif is_leaf:
                json.append('"%s%s" : ' % (prefix,name))
            else:
                json.append('"%s%s" :{' % (prefix,name))

            json.append(self._translate_node(is_collection, schema_root, element, prefixes))

            if not is_leaf:
                json.append('}')

            if not is_last:
                json.append(',')

            if prefix != "":
                prefixes.pop()

        if is_collection:
            json.append(']')
            json.append('}')
        elif is_list:
            json.append(']')            
        

        json.append("}")

        return ''.join(json)

    def _translate_node(self, is_collection, schema_root, xml_node, prefixes, depth=0):
        ''' Translates the given XML node into JSON

        is_collection -- True if the results is in the collection+json format
        xml_node -- the current XML element to translate
        '''
        json = list()
        first_child = False

        current_list = None
        


        siblings = collect_siblings(xml_node)

        if len(siblings) == 0:
            return '"%s"' % xml_node.text

        for sibling_tag, is_last in iterate_with_lookahead(siblings):
            
            sib_xml_nodes = siblings[sibling_tag]            

            child_schema_node = find_child_by_name(schema_root, sibling_tag)

            has_siblings = child_schema_node.is_listy()

            if has_siblings:
                sib_name, sib_prefix, prefixes = _split_tag_and_update_prefixes(sib_xml_nodes[0], prefixes)

                json.append('"%s%s" : [' % (sib_prefix, sib_name))

            for child_xml_node, sib_is_last  in iterate_with_lookahead(sib_xml_nodes):

                name, prefix, prefixes = _split_tag_and_update_prefixes(child_xml_node, prefixes)
                value = child_xml_node.text
                if not child_schema_node.is_leafy():
                    # it's a container, so iterate over children
                    if has_siblings:
                        json.append('{')
                    else:
                        json.append('"%s%s":{' % (prefix,name))
    
                    json.append(self._translate_node(is_collection,
                                                     child_schema_node,
                                                     child_xml_node,
                                                     prefixes,
                                                     depth+1))
    
                    json.append('}')
                            
                else:
                    if child_schema_node.node_type().leaf_type == RwYang.LeafType.LEAF_TYPE_EMPTY:
                        value = "[null]"
                    else:
                        if not value:
                            value = '""'
                        else:
                            try:
                                float(value)
                            except ValueError:
                                value = tornado.escape.json_encode(value)


                    
                    if has_siblings:
                        json.append('%s' % (value))
                    else:
                        json.append('"%s" : %s' % (name, value))

                if not sib_is_last:
                    json.append(',')

                if prefix != "":
                    prefixes.pop()

            if has_siblings:
                json.append(']')
                if sib_prefix != "":
                    prefixes.pop()        

            if not is_last:
                json.append(',')

        return ''.join(json)

def convert_xml_to_collection(url, xml_string):
    '''Translate the XML into a collection+xml format

    This strips off the elements corresponding to the given url and wraps the 
    remainder in a "collection" element.

    url -- the url corresponding to the query results being translated
    xml_string -- the XML results to translate    
    '''
    def find_collection_start(url_pieces, xml_node):
        ''' Walk down the XML tree and strip off the pieces coresponding to the url
        url_pieces -- the remaining url being converted, split on  "/"
        xml_node -- the current XML element to transform
        '''
        xml = list()
        if url_pieces:
            current_piece = url_pieces.pop(0)
            for child in xml_node:
                xml.append(find_collection_start(url_pieces, child))
        else:
            xml.append(lxml.etree.tostring(xml_node).decode("utf-8"))            

        return ''.join(xml)

    url_pieces, *_ = split_url(url)
    
    root = lxml.etree.fromstring(xml_string)    

    pruned_xml = find_collection_start(url_pieces, root)

    return "<collection>%s</collection>" % pruned_xml

def convert_netconf_response_to_json(xml_string):
    def walk(xml_node, depth=0):
        
        json = list()

        children = xml_node.getchildren()

        name = xml_node.tag.split("}")[-1]
        body = xml_node.text
        if body is not None:
            body = tornado.escape.json_encode(body)

        has_children = len(children) > 0

        if has_children:
            json.append('"%s" : {' % name)
        else:
            if body is not None:
                json.append('"%s" : %s' % (name, body))
            else:
                json.append('"%s" : ""' % (name))

        for child, is_last in iterate_with_lookahead(children):
            
            name = xml_node.tag.split("{")[-1]
            body = xml_node.text

            json.append(walk(child, depth + 1))

            if not is_last:
                json.append(',')

        if has_children:
            json.append('}')


        return ''.join(json)

            
    root_xml_node = lxml.etree.fromstring(xml_string)
    root_name = get_xml_element_name(root_xml_node)
    
    if root_name == "root":
        root_xml_node = root_xml_node[0]
        root_name = get_xml_element_name(root_xml_node)

    json_string = walk(root_xml_node)

    return "{%s}" % json_string

def convert_rpc_to_xml_output(xml):

    pos_data_tag = xml.find(r'<rpc-reply', 0)
    pos_data_start = xml.find(r'>', pos_data_tag) + 1
    pos_data_end = xml.rfind(r'</rpc-reply', pos_data_start)
    if pos_data_start >= 0 and pos_data_end >= 0:
        xml = xml[pos_data_start:pos_data_end]

    return "<output>%s</output>" % xml

def convert_rpc_to_json_output(json_string):
    json_dict = json.loads(json_string)
    json_dict['output'] = json_dict.pop('rpc-reply')

    return json.dumps(json_dict)

        
