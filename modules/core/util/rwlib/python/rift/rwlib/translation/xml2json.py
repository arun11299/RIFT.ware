# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Creation Date: 11/9/15
# 

from lxml import etree 

from ..schema import collect_children
from ..util import iterate_with_lookahead
from ..xml import (
    collect_siblings,
    get_xml_element_name,
)

def xml_to_json(schema_root, xml_string):
    '''Used to convert rooted XML into equivalent JSON 
    
    This class walks the xml tree and emits the equivalent json.
    
    '''
    def walk(schema_node, xml_node, depth=1):
        
        json = list()
        indent = "  " * depth
        
        siblings = collect_siblings(xml_node)

        for child_name, is_last in iterate_with_lookahead(siblings):

            sibling_xml_nodes = siblings[child_name]
    
            child_schema_node = schema_node.search_child(child_name)
            json.append('%s"%s:%s" : ' % (indent, child_schema_node.get_prefix(), child_schema_node.get_name()))

            is_leafy = child_schema_node.is_leafy()
            is_listy = child_schema_node.is_listy()

            if is_leafy and is_listy:
                json.append("%s[" % indent)
                list_indent = "  " + indent
                for sibling_xml_node, sibling_is_last in iterate_with_lookahead(sibling_xml_nodes):
                    json.append('%s"%s"' % (list_indent, sibling_xml_node.text))

                    if not sibling_is_last:
                        json[-1] += (",")
                    
                json.append("%s]" % indent)
                
            elif is_leafy:
                json.append('%s"%s"' % (indent, sibling_xml_nodes[0].text))

            elif is_listy:
                json.append("%s[" % indent)
                list_indent = "  " + indent
                for sibling_xml_node, sibling_is_last in iterate_with_lookahead(sibling_xml_nodes):
                    json.append("%s{" % list_indent)

                    json.append(walk(child_schema_node, sibling_xml_node, depth + 2))

                    json.append("%s}" % list_indent)

                    if not sibling_is_last:
                        json[-1] += (",")

                json.append("%s]" % indent)

            else: # it's a container
                json.append("%s{" % indent)

                json.append(walk(child_schema_node, sibling_xml_nodes[0], depth + 1))

                json.append("%s}" % indent)

            if not is_last:
                json[-1] += (",")

        return '\n'.join(json)
            
    if not (xml_string.startswith("<root") or xml_string.startswith("<data")):
        xml_string = "<root>%s</root>" % xml_string        

    root_xml_node = etree.fromstring(xml_string)
    
    json_string = walk(schema_root, root_xml_node)

    return "{\n" + json_string + "\n}"
