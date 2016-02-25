
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import asyncio
import sys

from gi.repository import (
    RwDts as rwdts,
    RwYang,
    RwResourceMgrYang,
    RwLaunchpadYang,
    RwcalYang,
)

from gi.repository.RwTypes import RwStatus
import rift.tasklets

if sys.version_info < (3, 4, 4):
    asyncio.ensure_future = asyncio.async


class ResourceMgrEvent(object):
    VDU_REQUEST_XPATH = "D,/rw-resource-mgr:resource-mgmt/vdu-event/vdu-event-data"
    VLINK_REQUEST_XPATH = "D,/rw-resource-mgr:resource-mgmt/vlink-event/vlink-event-data"

    def __init__(self, dts, log, loop, parent):
        self._log = log
        self._dts = dts
        self._loop = loop
        self._parent = parent
        self._vdu_reg = None
        self._link_reg = None

        self._vdu_reg_event = asyncio.Event(loop=self._loop)
        self._link_reg_event = asyncio.Event(loop=self._loop)

    @asyncio.coroutine
    def wait_ready(self, timeout=5):
        self._log.debug("Waiting for all request registrations to become ready.")
        yield from asyncio.wait([self._link_reg_event.wait(), self._vdu_reg_event.wait()],
                                timeout=timeout, loop=self._loop)

    @asyncio.coroutine
    def create_record_dts(self, regh, xact, path, msg):
        """
        Create a record in DTS with path and message
        """
        self._log.debug("Creating Resource Record xact = %s, %s:%s",
                        xact, path, msg)
        regh.create_element(path, msg)

    @asyncio.coroutine
    def delete_record_dts(self, regh, xact, path):
        """
        Delete a VNFR record in DTS with path and message
        """
        self._log.debug("Deleting Resource Record xact = %s, %s",
                        xact, path)
        regh.delete_element(path)


    @asyncio.coroutine
    def register(self):
        def on_link_request_commit(xact_info):
            """ The transaction has been committed """
            self._log.debug("Received link request commit (xact_info: %s)", xact_info)
            return rwdts.MemberRspCode.ACTION_OK

        @asyncio.coroutine
        def on_link_request_prepare(xact_info, action, ks_path, request_msg):
            self._log.debug("Received virtual-link on_prepare callback (xact_info: %s, action: %s): %s",
                            xact_info, action, request_msg)

            response_info = None
            response_xpath = ks_path.to_xpath(RwResourceMgrYang.get_schema()) + "/resource-info"

            schema = RwResourceMgrYang.VirtualLinkEventData().schema()
            pathentry = schema.keyspec_to_entry(ks_path)

            if action == rwdts.QueryAction.CREATE:
                response_info = yield from self._parent.allocate_virtual_network(pathentry.key00.event_id,
                                                                                 request_msg.cloud_account,
                                                                                 request_msg.request_info)
                #self.create_record_dts(self._link_reg, xact_info, response_xpath, response_info)
            elif action == rwdts.QueryAction.DELETE:
                yield from self._parent.release_virtual_network(pathentry.key00.event_id)
            elif action == rwdts.QueryAction.READ:
                response_info = yield from self._parent.read_virtual_network_info(pathentry.key00.event_id)
            else:
                raise ValueError("Only read/create/delete actions available. Received action: %s" %(action))

            self._log.debug("Responding with VirtualLinkInfo at xpath %s: %s.",
                            response_xpath, response_info)

            xact_info.respond_xpath(rwdts.XactRspCode.ACK, response_xpath, response_info)


        def on_vdu_request_commit(xact_info):
            """ The transaction has been committed """
            self._log.debug("Received vdu request commit (xact_info: %s)", xact_info)
            return rwdts.MemberRspCode.ACTION_OK

        def monitor_vdu_state(response_xpath, pathentry):
            self._log.info("Initiating VDU state monitoring for xpath: %s ", response_xpath)
            loop_cnt = 120
            while loop_cnt > 0:
                self._log.debug("VDU state monitoring: Sleeping for 1 second ")
                yield from asyncio.sleep(1, loop = self._loop)
                try:
                    response_info = yield from self._parent.read_virtual_compute_info(pathentry.key00.event_id)
                except Exception as e:
                    self._log.info("VDU state monitoring: Received exception %s in VDU state monitoring for %s. Aborting monitoring",
                                   str(e),response_xpath)
                    return
                if response_info.resource_state == 'active' or response_info.resource_state == 'failed':
                    self._log.info("VDU state monitoring: VDU reached terminal state. Publishing VDU info: %s at path: %s",
                                   response_info, response_xpath)
                    yield from self._dts.query_update(response_xpath,
                                                      rwdts.Flag.ADVISE,
                                                      response_info)
                    return
                else:
                    loop_cnt -= 1
            ### End of while loop. This is only possible if VDU did not reach active state
            self._log.info("VDU state monitoring: VDU at xpath :%s did not reached active state in 120 seconds. Aborting monitoring",
                           response_xpath)
            response_info = RwResourceMgrYang.VDUEventData_ResourceInfo()
            response_info.resource_state = 'failed'
            yield from self._dts.query_update(response_xpath,
                                              rwdts.Flag.ADVISE,
                                              response_info)
            return

        @asyncio.coroutine
        def on_vdu_request_prepare(xact_info, action, ks_path, request_msg):
            self._log.debug("Received vdu on_prepare callback (xact_info: %s, action: %s): %s",
                            xact_info, action, request_msg)

            response_info = None
            response_xpath = ks_path.to_xpath(RwResourceMgrYang.get_schema()) + "/resource-info"

            schema = RwResourceMgrYang.VDUEventData().schema()
            pathentry = schema.keyspec_to_entry(ks_path)

            if action == rwdts.QueryAction.CREATE:
                response_info = yield from self._parent.allocate_virtual_compute(pathentry.key00.event_id,
                                                                                 request_msg.cloud_account,
                                                                                 request_msg.request_info,
                                                                                 )
                if response_info.resource_state == 'pending':
                    asyncio.ensure_future(monitor_vdu_state(response_xpath, pathentry),
                                          loop = self._loop)

            elif action == rwdts.QueryAction.DELETE:
                yield from self._parent.release_virtual_compute(pathentry.key00.event_id)
            elif action == rwdts.QueryAction.READ:
                response_info = yield from self._parent.read_virtual_compute_info(pathentry.key00.event_id)
            else:
                raise ValueError("Only create/delete actions available. Received action: %s" %(action))

            self._log.debug("Responding with VDUInfo at xpath %s: %s",
                            response_xpath, response_info)

            xact_info.respond_xpath(rwdts.XactRspCode.ACK, response_xpath, response_info)



        @asyncio.coroutine
        def on_request_ready(registration, status):
            self._log.debug("Got request ready event (registration: %s) (status: %s)",
                            registration, status)

            if registration == self._link_reg:
                self._link_reg_event.set()
            elif registration == self._vdu_reg:
                self._vdu_reg_event.set()
            else:
                self._log.error("Unknown registration ready event: %s", registration)


        with self._dts.group_create() as group:
            self._log.debug("Registering for Link Resource Request using xpath: %s",
                            ResourceMgrEvent.VLINK_REQUEST_XPATH)

            self._link_reg = group.register(xpath=ResourceMgrEvent.VLINK_REQUEST_XPATH,
                                            handler=rift.tasklets.DTS.RegistrationHandler(on_ready=on_request_ready,
                                                                                          on_commit=on_link_request_commit,
                                                                                          on_prepare=on_link_request_prepare),
                                            flags=rwdts.Flag.PUBLISHER)

            self._log.debug("Registering for VDU Resource Request using xpath: %s",
                            ResourceMgrEvent.VDU_REQUEST_XPATH)

            self._vdu_reg = group.register(xpath=ResourceMgrEvent.VDU_REQUEST_XPATH,
                                           handler=rift.tasklets.DTS.RegistrationHandler(on_ready=on_request_ready,
                                                                                         on_commit=on_vdu_request_commit,
                                                                                         on_prepare=on_vdu_request_prepare),
                                           flags=rwdts.Flag.PUBLISHER)

