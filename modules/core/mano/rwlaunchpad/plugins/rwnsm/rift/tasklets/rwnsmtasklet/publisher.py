# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import asyncio

from gi.repository import (
    RwDts as rwdts,
    )
import rift.tasklets


class NsrOpDataDtsHandler(object):
    """ The network service op data DTS handler """
    XPATH = "D,/nsr:ns-instance-opdata/nsr:nsr"

    def __init__(self, dts, log, loop):
        self._dts = dts
        self._log = log
        self._loop = loop
        self._regh = None

    @property
    def regh(self):
        """ Return the registration handle"""
        return self._regh

    @asyncio.coroutine
    def register(self):
        """ Register for Nsr op data publisher registration"""
        self._log.debug("Registering Nsr op data path %s as publisher",
                        NsrOpDataDtsHandler.XPATH)

        hdl = rift.tasklets.DTS.RegistrationHandler()
        with self._dts.group_create() as group:
            self._regh = group.register(xpath=NsrOpDataDtsHandler.XPATH,
                                        handler=hdl,
                                        flags=rwdts.Flag.PUBLISHER | rwdts.Flag.NO_PREP_READ)

    @asyncio.coroutine
    def create(self, xact, path, msg):
        """
        Create an NS record in DTS with the path and message
        """
        self._log.debug("Creating NSR xact = %s, %s:%s", xact, path, msg)
        self.regh.create_element(path, msg)
        self._log.debug("Created NSR xact = %s, %s:%s", xact, path, msg)

    @asyncio.coroutine
    def update(self, xact, path, msg, flags=rwdts.Flag.REPLACE):
        """
        Update an NS record in DTS with the path and message
        """
        self._log.debug("Updating NSR xact = %s, %s:%s regh = %s", xact, path, msg, self.regh)
        self.regh.update_element(path, msg, flags)
        self._log.debug("Updated NSR xact = %s, %s:%s", xact, path, msg)

    @asyncio.coroutine
    def delete(self, xact, path):
        """
        Update an NS record in DTS with the path and message
        """
        self._log.debug("Deleting NSR xact:%s, path:%s", xact, path)
        self.regh.delete_element(path)
        self._log.debug("Deleted NSR xact:%s, path:%s", xact, path)



class VnfrPublisherDtsHandler(object):
    """ Registers 'D,/vnfr:vnfr-catalog/vnfr:vnfr' DTS"""
    XPATH = "D,/vnfr:vnfr-catalog/vnfr:vnfr"

    def __init__(self, dts, log, loop):
        self._dts = dts
        self._log = log
        self._loop = loop

        self._regh = None

    @property
    def regh(self):
        """ Return registration handle"""
        return self._regh

    @asyncio.coroutine
    def register(self):
        """ Register for Vvnfr create/update/delete/read requests from dts """

        @asyncio.coroutine
        def on_prepare(xact_info, action, ks_path, msg):
            """ prepare callback from dts """
            self._log.debug(
                "Got vnfr on_prepare callback (xact_info: %s, action: %s): %s",
                xact_info, action, msg
                )
            raise NotImplementedError(
                "%s action on VirtualNetworkFunctionRecord not supported",
                action)

        self._log.debug("Registering for VNFR using xpath: %s",
                        VnfrPublisherDtsHandler.XPATH,)

        hdl = rift.tasklets.DTS.RegistrationHandler()
        with self._dts.group_create() as group:
            self._regh = group.register(xpath=VnfrPublisherDtsHandler.XPATH,
                                        handler=hdl,
                                        flags=(rwdts.Flag.PUBLISHER |
                                               rwdts.Flag.NO_PREP_READ |
                                               rwdts.Flag.CACHE),)

    @asyncio.coroutine
    def create(self, xact, path, msg):
        """
        Create a VNFR record in DTS with path and message
        """
        self._log.debug("Creating VNFR xact = %s, %s:%s",
                        xact, path, msg)
        self.regh.create_element(path, msg)
        self._log.debug("Created VNFR xact = %s, %s:%s",
                        xact, path, msg)

    @asyncio.coroutine
    def update(self, xact, path, msg):
        """
        Update a VNFR record in DTS with path and message
        """
        self._log.debug("Updating VNFR xact = %s, %s:%s",
                        xact, path, msg)
        self.regh.update_element(path, msg)
        self._log.debug("Updated VNFR xact = %s, %s:%s",
                        xact, path, msg)

    @asyncio.coroutine
    def delete(self, xact, path):
        """
        Delete a VNFR record in DTS with path and message
        """
        self._log.debug("Deleting VNFR xact = %s, %s", xact, path)
        self.regh.delete_element(path)
        self._log.debug("Deleted VNFR xact = %s, %s", xact, path)


class VlrPublisherDtsHandler(object):
    """ registers 'D,/vlr:vlr-catalog/vlr:vlr """
    XPATH = "D,/vlr:vlr-catalog/vlr:vlr"

    def __init__(self, dts, log, loop):
        self._dts = dts
        self._log = log
        self._loop = loop

        self._regh = None

    @property
    def regh(self):
        """ Return registration handle"""
        return self._regh

    @asyncio.coroutine
    def register(self):
        """ Register for vlr create/update/delete/read requests from dts """

        @asyncio.coroutine
        def on_prepare(xact_info, action, ks_path, msg):
            """ prepare callback from dts """
            self._log.debug(
                "Got vlr on_prepare callback (xact_info: %s, action: %s): %s",
                xact_info, action, msg
                )
            raise NotImplementedError(
                "%s action on VirtualLinkRecord not supported",
                action)

        self._log.debug("Registering for VLR using xpath: %s",
                        VlrPublisherDtsHandler.XPATH,)

        hdl = rift.tasklets.DTS.RegistrationHandler()
        with self._dts.group_create() as group:
            self._regh = group.register(xpath=VlrPublisherDtsHandler.XPATH,
                                        handler=hdl,
                                        flags=(rwdts.Flag.PUBLISHER |
                                               rwdts.Flag.NO_PREP_READ |
                                               rwdts.Flag.CACHE),)

    @asyncio.coroutine
    def create(self, xact, path, msg):
        """
        Create a VLR record in DTS with path and message
        """
        self._log.debug("Creating VLR xact = %s, %s:%s",
                        xact, path, msg)
        self.regh.create_element(path, msg)
        self._log.debug("Created VLR xact = %s, %s:%s",
                        xact, path, msg)

    @asyncio.coroutine
    def update(self, xact, path, msg):
        """
        Update a VLR record in DTS with path and message
        """
        self._log.debug("Updating VLR xact = %s, %s:%s",
                        xact, path, msg)
        self.regh.update_element(path, msg)
        self._log.debug("Updated VLR xact = %s, %s:%s",
                        xact, path, msg)

    @asyncio.coroutine
    def delete(self, xact, path):
        """
        Delete a VLR record in DTS with path and message
        """
        self._log.debug("Deleting VLR xact = %s, %s", xact, path)
        self.regh.delete_element(path)
        self._log.debug("Deleted VLR xact = %s, %s", xact, path)
