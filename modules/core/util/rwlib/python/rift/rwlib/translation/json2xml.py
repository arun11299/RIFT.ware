# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Creation Date: 9/13/15
# 

import json

from ..schema import collect_children

def json_to_xml(schema_node, json_string):
    '''Used to convert rooted JSON into equivalent XML 
    
    The json library's representation of JSON can be thought of as a tree of
    dicts, lists, and leaves. This function walks that tree shape and emits the
    equivalent XML.
    '''
    def _handle_dict(json_node, schema_node, key=None): 
        '''Translate a dict representaion of JSON into XML

        In the json library dict's hold all the data. They either contain more
        "subtrees", or they map names to the value that is being set.

        In  rpcs the order the inputs are specified has to match the order
        they are defined in the yang file, so this will emit all XML in
        that order.

        json_node -- the current subtree being translated

        '''
        xml = list()

        schema_children = collect_children(schema_node)

        for child_schema_node in schema_children:

            schema_prefix = child_schema_node.get_prefix()
            schema_namespace = child_schema_node.get_ns()
            schema_node_name = child_schema_node.get_name()

            try:
                key = schema_node_name
                values = json_node[schema_node_name]
            except KeyError:
                try:
                    prefixed_name = "%s:%s" % (schema_prefix, schema_node_name)
                    values = json_node[prefixed_name]
                except KeyError:
                    continue

            # recurse
            if type(values) == list:
                xml.append(_handle_list(values, child_schema_node, key))
            elif child_schema_node.get_name() == "input" and child_schema_node.is_rpc_input():
                # don't specify "input" element
                xml.append(handler[type(values)](values, child_schema_node, key))
            else:
                xml.append('<%s xmlns="%s">' % (schema_node_name, schema_namespace))
                xml.append(handler[type(values)](values, child_schema_node, key))
                xml.append("</%s>" % (schema_node_name))
                
        return ''.join(xml)

    def _handle_list(json_node, schema_node, key=None):
        # lists only aggregate other structures
        xml = list()

        for item in json_node:
            xml.append('<%s >' % (key))
            xml.append(handler[type(item)](item, schema_node))
            xml.append("</%s>" % (key))

        return ''.join(xml)

    def _handle_int(json_node, schema_node, key=None):
        return str(json_node)

    def _handle_str(json_node, schema_node, key=None):
        return json_node

    handler = {dict : _handle_dict, 
               int : _handle_int, 
               bool : _handle_int,
               str : _handle_str}        

    parsed_json= json.loads(json_string)

    return "<root>" + handler[type(parsed_json)](parsed_json, schema_node) + "</root>"
