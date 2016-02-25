#!/usr/bin/env python3

# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import argparse
import logging
import rift.auto.proxy
import rift.vcs
import sys

from gi.repository import NsdYang, VldYang, VnfdYang, RwYang

logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(__name__)

model = RwYang.Model.create_libncx()
model.load_schema_ypbc(VldYang.get_schema())
model.load_schema_ypbc(NsdYang.get_schema())
model.load_schema_ypbc(VnfdYang.get_schema())


def configure_vld(proxy, vld_xml_hdl):
    vld_xml = vld_xml_hdl.read()
    logger.debug("Attempting to deserialize XML into VLD protobuf: %s", vld_xml)
    vld = VldYang.YangData_Vld_VldCatalog_Vld()
    vld.from_xml_v2(model, vld_xml)

    logger.debug("Sending VLD to netconf: %s", vld)
    proxy.merge_config(vld.to_xml_v2(model))


def configure_vnfd(proxy, vnfd_xml_hdl):
    vnfd_xml = vnfd_xml_hdl.read()
    logger.debug("Attempting to deserialize XML into VNFD protobuf: %s", vnfd_xml)
    vnfd = VnfdYang.YangData_Vnfd_VnfdCatalog_Vnfd()
    vnfd.from_xml_v2(model, vnfd_xml)

    logger.debug("Sending VNFD to netconf: %s", vnfd)
    proxy.merge_config(vnfd.to_xml_v2(model))


def configure_nsd(proxy, nsd_xml_hdl):
    nsd_xml = nsd_xml_hdl.read()
    logger.debug("Attempting to deserialize XML into NSD protobuf: %s", nsd_xml)
    nsd = NsdYang.YangData_Nsd_NsdCatalog_Nsd()
    nsd.from_xml_v2(model, nsd_xml)

    logger.debug("Sending NSD to netconf: %s", nsd)
    proxy.merge_config(nsd.to_xml_v2(model))


def parse_args(argv=sys.argv[1:]):
    """Create a parser which includes all generic demo arguments and parse args

    Arguments:
        argv - arguments to be parsed

    Returns: List of parsed arguments
    """

    parser = argparse.ArgumentParser()
    parser.add_argument(
            '--confd-host',
            default="127.0.0.1",
            help="Hostname or IP where the confd netconf server is running.")

    parser.add_argument(
            '--vld-xml-file',
            action="append",
            default=[],
            type=argparse.FileType(),
            help="VLD XML File Path",
            )

    parser.add_argument(
            '--vnfd-xml-file',
            action="append",
            default=[],
            type=argparse.FileType(),
            help="VNFD XML File Path",
            )

    parser.add_argument(
            '--nsd-xml-file',
            action="append",
            default=[],
            type=argparse.FileType(),
            help="VNFD XML File Path",
            )

    parser.add_argument(
            '-v', '--verbose',
            action='store_true',
            help="Logging is normally set to an INFO level. When this flag "
                 "is used logging is set to DEBUG. ")

    args = parser.parse_args(argv)

    return args


def connect(args):
    # Initialize Netconf Management Proxy
    mgmt_proxy = rift.auto.proxy.NetconfProxy(args.confd_host)
    mgmt_proxy.connect()

    # Ensure system started
    vcs_component_info = rift.vcs.mgmt.VcsComponentInfo(mgmt_proxy)
    vcs_component_info.wait_until_system_started()

    return mgmt_proxy


def main():
    args = parse_args()
    proxy = connect(args)
    for xml_file in args.vnfd_xml_file:
        configure_vnfd(proxy, xml_file)

    for xml_file in args.vld_xml_file:
        configure_vld(proxy, xml_file)

    for xml_file in args.nsd_xml_file:
        configure_nsd(proxy, xml_file)


if __name__ == "__main__":
    main()

