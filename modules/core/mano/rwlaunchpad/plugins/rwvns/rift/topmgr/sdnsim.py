
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

from . import core
import logging

import xml.etree.ElementTree as etree
import json
from gi.repository import RwTopologyYang as RwTl
from gi.repository import RwYang


logger = logging.getLogger(__name__)


class SdnSim(core.Topology):
    def __init__(self):
        super(SdnSim, self).__init__()

    def get_network_list(self, account):
        """
        Returns the discovered network

        @param account - a SDN account

        """
        topology_source = "/net/boson/home1/rchamart/work/topology/l2_top.xml"
        logger.info("Reading topology file: %s", topology_source)
        tree = etree.parse(topology_source)
        root = tree.getroot()
        xmlstr = etree.tostring(root, encoding="unicode")

        model = RwYang.Model.create_libncx()
        model.load_schema_ypbc(RwTl.get_schema())
        nwtop = RwTl.YangData_IetfNetwork()
        # The top level topology object does not have XML conversion
        # Hence going one level down
        l2nw1 = nwtop.network.add()
        l2nw1.from_xml_v2(model, xmlstr)

        logger.debug("Returning topology data imported from XML file")

        return nwtop
