
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import collections
import itertools
import logging
import os
import uuid
import time

import ipaddress

from gi.repository import (
    GObject,
    RwSdn, # Vala package
    RwTypes,
    RwsdnYang,
    #IetfL2TopologyYang as l2Tl,
    RwTopologyYang as RwTl,
    )

import rw_status
import rwlogger

from rift.topmgr.sdnsim import SdnSim


logger = logging.getLogger('rwsdn.sdnsim')


class UnknownAccountError(Exception):
    pass


class MissingFileError(Exception):
    pass


rwstatus = rw_status.rwstatus_from_exc_map({
    IndexError: RwTypes.RwStatus.NOTFOUND,
    KeyError: RwTypes.RwStatus.NOTFOUND,
    UnknownAccountError: RwTypes.RwStatus.NOTFOUND,
    MissingFileError: RwTypes.RwStatus.NOTFOUND,
    })


class SdnSimPlugin(GObject.Object, RwSdn.Topology):

    def __init__(self):
        GObject.Object.__init__(self)
        self.sdnsim = SdnSim()
        

    @rwstatus
    def do_init(self, rwlog_ctx):
        if not any(isinstance(h, rwlogger.RwLogger) for h in logger.handlers):
            logger.addHandler(
                rwlogger.RwLogger(
                    category="sdnsim",
                    log_hdl=rwlog_ctx,
                )
            )

    @rwstatus(ret_on_failure=[None])
    def do_get_network_list(self, account):
        """
        Returns the list of discovered networks

        @param account - a SDN account

        """
        logger.debug('Get network list: ')
        nwtop = self.sdnsim.get_network_list( account)
        logger.debug('Done with get network list: %s', type(nwtop))
        return nwtop
