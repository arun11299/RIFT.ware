# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Creation Date: 11/9/15
# 

def collect_children(node):
    children = list()
    child = node.get_first_child()

    while child is not None:
        children.append(child)

        child = child.get_next_sibling()

    return children

def get_keys(schema_node):
    assert(schema_node.is_listy())
    
    keys = list()
    cur_key = schema_node.get_first_key()

    while cur_key is not None:
        keys.append(cur_key.get_key_node())

        cur_key = cur_key.next_key

    return keys

def get_key_names(schema_node):
    assert(schema_node.is_listy())
    
    keys = list()
    cur_key = schema_node.get_first_key()

    while cur_key is not None:
        key_name = cur_key.get_key_node().get_name()
        keys.append(key_name)
        cur_key = cur_key.next_key

    return keys
