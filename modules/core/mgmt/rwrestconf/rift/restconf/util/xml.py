# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Max Beckett
# Creation Date: 9/5/2015
# 

import lxml.etree

from .schema import find_child_by_name
from .web import split_url

def create_xpath_from_url(url, schema):

    block = "/*[name()='%s']"

    def _build_xpath(url_pieces, schema_node):
        if not url_pieces:
            return ""

        current_piece = url_pieces.pop(0)

        child_schema_node = find_child_by_name(schema_node, current_piece)
        piece_is_key = False

        if schema_node.is_listy():
            # it's a key
            piece_is_key = True

            if len(url_pieces) == 0:
                return ""

            current_piece = url_pieces.pop(0)
            child_schema_node = find_child_by_name(schema_node, current_piece)

        xpath_snippet = block % current_piece
        return xpath_snippet + _build_xpath(url_pieces, child_schema_node)

    url_pieces, *_ = split_url(url)

    xpath = _build_xpath(url_pieces, schema)

    xpath = "/*[name()='data']" + xpath

    return xpath
