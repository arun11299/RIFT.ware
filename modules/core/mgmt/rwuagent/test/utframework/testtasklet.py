"""
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

@file testtasklet.py
@author Varun Prasad
@date 2015-09-28
"""

import abc
import asyncio
import bisect
import collections
import functools
import functools
import itertools
import logging
import math
import sys
import time

import gi.repository.RwTypes as rwtypes
import rift.tasklets
import rift.tasklets.dts

from gi.repository import (
    RwDts as rwdts,
    InterfacesYang as interfaces,
    DnsYang as dns,
    RoutesYang as routes,
    NtpYang as ntp,
    NotifYang,
    UtCompositeYang as composite,
    RwVcsYang as rwvcs
)

from gi.repository import RwMgmtagtYang

if sys.version_info < (3, 4, 4):
    asyncio.ensure_future = asyncio.async


class TaskletState(object):
    """
    Different states of the tasklet.
    """
    STARTING, RUNNING, FINISHED = ("starting", "running", "finished")


class TestTasklet(rift.tasklets.Tasklet):
    def start(self):
        """Entry point for tasklet
        """
        self.log.setLevel(logging.INFO)
        super().start()

        self._dts = rift.tasklets.DTS(
                self.tasklet_info,
                composite.get_schema(),
                self._loop,
                self.on_dts_state_change)

        # Set the instance id
        self.instance_name = self.tasklet_info.instance_name
        self.instance_id = int(self.instance_name.rsplit('-', 1)[1])
        self.interfaces = {}
        self.log.debug("Starting TestTasklet Name: {}, Id: {}".format(
                self.instance_name,
                self.instance_id))

        self.state = TaskletState.STARTING

    def stop(self):
        """Exit point for the tasklet
        """
        # All done!
        super().stop()

    @asyncio.coroutine
    def on_dts_state_change(self, state):
        """Take action according to current dts state to transition
        application into the corresponding application state

        Args:
            state (rwdts.State): Description
        """

        switch = {
            rwdts.State.CONFIG: rwdts.State.INIT,
            rwdts.State.INIT: rwdts.State.REGN_COMPLETE,
            rwdts.State.REGN_COMPLETE: rwdts.State.RUN,
        }

        handlers = {
            rwdts.State.INIT: self.init,
            rwdts.State.RUN: self.run,
        }

        # Transition application to next state
        handler = handlers.get(state, None)
        if handler is not None:
            yield from handler()

        # Transition dts to next state
        next_state = switch.get(state, None)
        self.log.info("DTS transition from {} -> {}".format(state, next_state))

        if next_state is not None:
            self._dts.handle.set_state(next_state)

    # Callbacks
    @asyncio.coroutine
    def init(self):
        """Initialize application. During this state transition all DTS
        registrations and subscriptions required by application
        should be started.
        """

        @asyncio.coroutine
        def interface_prepare_config(dts, acg, xact, xact_info, ksp, msg):
            """Prepare for application configuration.
            """
            self.log.debug("Prepare Callback")
            # Store the interfaces
            self.interfaces[msg.name] = msg
            acg.handle.prepare_complete_ok(xact_info.handle)

        @asyncio.coroutine
        def routes_prepare_config(dts, acg, xact, xact_info, ksp, msg):
            """Prepare for application configuration.
            """
            self.log.debug("Prepare Callback")
            acg.handle.prepare_complete_ok(xact_info.handle)

        @asyncio.coroutine
        def dns_prepare_config(dts, acg, xact, xact_info, ksp, msg):
            """Prepare for application configuration.
            """
            self.log.debug("Prepare Callback")
            acg.handle.prepare_complete_ok(xact_info.handle)

        @asyncio.coroutine
        def ntp_prepare_config(dts, acg, xact, xact_info, ksp, msg):
            """Prepare for application configuration.
            """
            self.log.debug("Prepare Callback")
            acg.handle.prepare_complete_ok(xact_info.handle)

        def apply_config(dts, acg, xact, action, scratch):
            """On apply callback for AppConf registration"""
            self.log.debug("Apply Config")
            return rwtypes.RwStatus.SUCCESS

        @asyncio.coroutine
        def interface_status(xact_info, action, ks_path, msg):
            xpath = "D,/interfaces:interfaces/interfaces:interface[interfaces:name='eth0']"
            interf = interfaces.Interface()
            interf.name = "eth0"
            interf.status.link = "up"
            interf.status.speed = "hundred"
            interf.status.duplex = "full"
            interf.status.mtu = 1500
            interf.status.receive.bytes = 1234567
            interf.status.receive.packets = 1234
            interf.status.receive.errors = 0
            interf.status.receive.dropped = 100
            xact_info.respond_xpath(rwdts.XactRspCode.ACK, xpath, interf)

        @asyncio.coroutine
        def ntp_status(xact_info, action, ks_path, msg):
            xpath = "D,/ntp:ntp/ntp:server[ntp:name='server1']/ntp:status"
            ntp_status = NtpYang.Server.Status()
            ntp_status.state = "selected"
            ntp_status.stratum = 8
            ntp_status.reach = "xyz"
            ntp_status.delay = 100
            ntp_status.offset = 10
            ntp_status.jitter = 20
            xact_info.respond_xpath(rwdts.XactRspCode.ACK, xpath, ntp_status)

        @asyncio.coroutine
        def clear_interface(xact_info, action, ks_path, msg):
            xpath = "O,/interfaces:clear-interface"
            op=interfaces.ClearInterfaceOp()
            op.status="Success"
            xact_info.respond_xpath(rwdts.XactRspCode.ACK, xpath, op)

        @asyncio.coroutine
        def configure_via_dts_rpc(xact_info, action, ks_path, msg):
            self.log.debug("RECEIVED CONFIG VIS DTS RPC")
            pb = ntp.Ntp()
            server = pb.server.add()
            server.name="10.0.1.9"
            server.version=3
            ec_rpc=RwMgmtagtYang.YangInput_RwMgmtagt_MgmtAgent()
            ec_rpc.pb_request.xpath="C,/ntp:ntp"
            ec_rpc.pb_request.request_type="edit_config"
            ec_rpc.pb_request.edit_type="merge"
            ec_rpc.pb_request.data=pb.to_pbuf()
            query_iter = yield from self._dts.query_rpc(
                            xpath="I,/rw-mgmtagt:mgmt-agent",
                            flags=0,
                            msg=ec_rpc)
            for fut_resp in query_iter:
               query_resp = yield from fut_resp
               result = query_resp.result
               self.log.debug("Agent rpc returned")

            xpath = "O,/ntp:configure-rpc"
            op=ntp.YangOutput_Ntp_ConfigureRpc()
            op.status="Success"
            xact_info.respond_xpath(rwdts.XactRspCode.ACK, xpath, op)

        @asyncio.coroutine
        def raise_new_route_notif(xact_info, action, ks_path, msg):
            xpath = "N,/notif:new-route"
            notif = NotifYang.YangNotif_Notif_NewRoute()
            notif.name = "New Route 1/2WX Added"
            yield from self._dts.query_create(xpath, rwdts.Flag.ADVISE, notif)
            yield from self._dts.query_update(xpath, rwdts.Flag.ADVISE, notif)
            
            xpath = "O,/interfaces:raise-new-route"
            op=interfaces.RaiseNewRouteOp()
            op.status="Success"
            xact_info.respond_xpath(rwdts.XactRspCode.ACK, xpath, op)


        @asyncio.coroutine
        def raise_route_notif(xact_info, action, ks_path, msg):
            xpath = "N,/notif:new-route"
            nroute = NotifYang.YangNotif_Notif_NewRoute()
            nroute.name = "New Route"
            xact_info.respond_xpath(rwdts.XactRspCode.ACK, xpath, nroute)
        
        @asyncio.coroutine
        def raise_temp_alarm_notif(xact_info, action, ks_path, msg):
            xpath = "N,/notif:temp-alarm" 
            alarm = NotifYang.YangNotif_Notif_TempAlarm()
            alarm.message = "Temperature above threshold"
            alarm.curr_temp = 100
            alarm.thresh_temp = 80
            yield from self._dts.query_create(xpath, rwdts.Flag.ADVISE, alarm)
            yield from self._dts.query_create(xpath, rwdts.Flag.ADVISE, alarm)

            xpath = "O,/interfaces:raise-temp-alarm"
            op=interfaces.RaiseTempAlarmOp()
            op.status="Success"
            xact_info.respond_xpath(rwdts.XactRspCode.ACK, xpath, op)

        @asyncio.coroutine
        def raise_temp_notif(xact_info, action, ks_path, msg):
            xpath = "N,/notif:temp-alarm"
            alarm = NotifYang.YangNotif_Notif_TempAlarm()
            alarm.message = "Temperature above threshold"
            alarm.curr_temp = 100
            alarm.thresh_temp = 80
            xact_info.respond_xpath(rwdts.XactRspCode.ACK, xpath, alarm)
            
        #Operational data
        yield from self._dts.register(
              flags=rwdts.Flag.PUBLISHER,
              xpath="D,/interfaces:interfaces/interfaces:interface",
              handler=rift.tasklets.DTS.RegistrationHandler(
                    on_prepare=interface_status))

        yield from self._dts.register(
              flags=rwdts.Flag.PUBLISHER,
              xpath="D,/ntp:ntp/ntp:server",
              handler=rift.tasklets.DTS.RegistrationHandler(
                    on_prepare=ntp_status))

        #RPC
        yield from self._dts.register(
              xpath="I,/interfaces:clear-interface",
              flags=rwdts.Flag.PUBLISHER,
              handler=rift.tasklets.DTS.RegistrationHandler(
                    on_prepare=clear_interface))

        yield from self._dts.register(
              xpath="I,/interfaces:raise-new-route",
              flags=rwdts.Flag.PUBLISHER,
              handler=rift.tasklets.DTS.RegistrationHandler(
                    on_prepare=raise_new_route_notif))

        yield from self._dts.register(
              xpath="I,/ntp:configure-rpc",
              flags=rwdts.Flag.PUBLISHER,
              handler=rift.tasklets.DTS.RegistrationHandler(
                     on_prepare=configure_via_dts_rpc))

        yield from self._dts.register(
              xpath="I,/interfaces:raise-temp-alarm",
              flags=rwdts.Flag.PUBLISHER,
              handler=rift.tasklets.DTS.RegistrationHandler(
                    on_prepare=raise_temp_alarm_notif))

        #Notification
        yield from self._dts.register(
              xpath="N,/notif:new-route",
              flags=rwdts.Flag.PUBLISHER,
              handler=rift.tasklets.DTS.RegistrationHandler(
                    on_prepare=raise_route_notif))

        yield from self._dts.register(
              xpath="N,/notif:temp-alarm",
              flags=rwdts.Flag.PUBLISHER,
              handler=rift.tasklets.DTS.RegistrationHandler(
                    on_prepare=raise_temp_notif))

        with self._dts.appconf_group_create(
                handler=rift.tasklets.AppConfGroup.Handler(
                        on_apply=apply_config)) as acg:
            acg.register(
                    xpath="C,/interfaces:interfaces/interfaces:interface",
                    flags=rwdts.Flag.SUBSCRIBER|rwdts.Flag.CACHE|0,
                    on_prepare=interface_prepare_config)

            acg.register(
                    xpath="C,/routes:routes",
                    flags=rwdts.Flag.SUBSCRIBER|rwdts.Flag.CACHE|0,
                    on_prepare=routes_prepare_config)

            acg.register(
                    xpath="C,/ntp:ntp",
                    flags=rwdts.Flag.SUBSCRIBER|rwdts.Flag.CACHE|0,
                    on_prepare=ntp_prepare_config)

            acg.register(
                    xpath="C,/dns:dns",
                    flags=rwdts.Flag.SUBSCRIBER|rwdts.Flag.CACHE|0,
                    on_prepare=dns_prepare_config)

    @asyncio.coroutine
    def run(self):
        """Initialize application. During this state transition all DTS
        registrations and subscriptions required by application should be started
        """
        yield from self._dts.ready.wait()
