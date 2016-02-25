#!/usr/bin/env python

# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Austin Cormier
# Creation Date: 12/01/2014
# 
#
# XML Utility functions

import xml.dom.minidom as xml_minidom

def pretty_xml(xml_str):
    """Creates a more human readable XML string by using
    new lines and indentation where appropriate.

    Arguments:
        xml_str - An ugly, but valid XML string.
    """
    return xml_minidom.parseString(xml_str).toprettyxml('  ')


