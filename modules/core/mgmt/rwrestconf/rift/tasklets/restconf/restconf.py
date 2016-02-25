# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Max Beckett
# Creation Date: 8/15/2015
# 

import asyncio
from enum import Enum
import itertools
import sys

import ncclient.asyncio_manager
import tornado

from gi.repository import (
    RwDts,
    RwDynSchema,
    RwRestconfYang,
    RwYang,
    RwMgmtSchemaYang,
)
import rift.tasklets
import gi.repository.RwTypes as rwtypes

from rift.restconf import (
    ConfdRestTranslator,
    Configuration,
    HttpHandler,
    ConnectionManager,
    XmlToJsonTranslator,
    Statistics,
    load_schema_root,
)

class SchemaState(Enum):
    working = 'working'
    waiting = 'waiting'
    ready = 'ready'
    initializing = 'initializing'
    error = 'error'

def dyn_schema_callback(instance, numel, module_names, fxs_filenames, so_filenames, yang_filenames):
    instance._schema_state = SchemaState.waiting
    for module_name, so_filename in zip(module_names, so_filenames):
        instance._pending_modules[module_name] = so_filename

    if not instance._initialized:
        instance._initialized = True
        instance._initialise_composite_and_start()
    
def _load_schema(schema_name):
    yang_model = RwYang.Model.create_libncx()
    schema = RwYang.Model.load_and_get_schema(schema_name)
    yang_model.load_schema_ypbc(schema)

    return schema

if sys.version_info < (3,4,4):
    asyncio.ensure_future = asyncio.async

class RestconfTasklet(rift.tasklets.Tasklet):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._server = None

    def start(self):
        """Tasklet entry point"""
        super(RestconfTasklet, self).start()

        self._tasklet_name = "RwRestconf"
        self._dynamic_schema_publish="D,/rw-mgmt-schema:rw-mgmt-schema-state/rw-mgmt-schema:listening-apps[name='%s']" % self._tasklet_name
        self._initialized = False
        self._schema_state = SchemaState.initializing
        self._dynamic_schema_response = RwMgmtSchemaYang.YangData_RwMgmtSchema_RwMgmtSchemaState_ListeningApps()
        self._dynamic_schema_response.name = self._tasklet_name
        self._dynamic_schema_response.state = self._schema_state.value
        self._dynamic_schema_response.app_type = "nb_interface"
        self._pending_modules = dict()
        self.get_stats = 0

        manifest = self.tasklet_info.get_pb_manifest()
        self.schema_name = "rw-restconf"#manifest.bootstrap_phase.rwbaseschema.schema_name

        self._dts = rift.tasklets.DTS(
            self.tasklet_info,
            RwRestconfYang.get_schema(),
            self.loop,
            self.on_dts_state_change)


    @asyncio.coroutine
    def on_dts_state_change(self, state):
        """Take action according to current dts state to transition
        application in to the corresponding application state
        """
        switch = {
            RwDts.State.INIT: RwDts.State.RUN,
        }

        handlers = {
            RwDts.State.INIT: self.init,
            RwDts.State.RUN: self.run,
        }

        # Transition application to next state
        handler = handlers[state]
        yield from handler()

        # Transition dts to next state
        try:
            next_state = switch[state]
        except KeyError:
            # we don't handle all state changes
            return

        self._dts.handle.set_state(next_state)

    @asyncio.coroutine
    def init(self):
        self._configuration = Configuration()
        self._statistics = Statistics()
        self._messages = {}

        @asyncio.coroutine
        def on_prepare(dts, acg, xact, xact_info, ksp, msg):
            self._messages[xact.id] = msg
            acg.handle.prepare_complete_ok(xact_info.handle)

        def on_apply(dts, acg, xact, action, scratch):
            if action == RwDts.AppconfAction.INSTALL and xact.id is None:
                return

            if xact.id not in self._messages:
                raise KeyError("No stashed configuration found with transaction id [{}]".format(xact.id))

            toggles = self._messages[xact.id]
            timing_value = toggles.log_timing

            self._configuration.log_timing = timing_value

            del self._messages[xact.id]
            
        with self._dts.appconf_group_create(
                handler=rift.tasklets.AppConfGroup.Handler(
                    on_apply=on_apply)) as acg:
            acg.register(
                xpath="C,/rw-restconf:rwrestconf-configuration",
                flags=RwDts.Flag.SUBSCRIBER,
                on_prepare=on_prepare)

            
        self._ready_for_schema = False

        @asyncio.coroutine
        def dynamic_schema_state(xact_info, action, ks_path, msg):
            self._dynamic_schema_response.state = self._schema_state.value
            xact_info.respond_xpath(RwDts.XactRspCode.ACK, self._dynamic_schema_publish, self._dynamic_schema_response)
            
        yield from self._dts.register(
                flags=RwDts.Flag.PUBLISHER,
                xpath=(self._dynamic_schema_publish),                       
                handler=rift.tasklets.DTS.RegistrationHandler(
                    on_prepare=dynamic_schema_state))

        @asyncio.coroutine
        def load_modules_prepare(xact_info, action, path, msg):
            if msg.state != "loading_nb_interfaces":
                xact_info.respond_xpath(RwDts.XactRspCode.ACK, path.create_string())
                return

            self._schema_state = SchemaState.working

            module_name = msg.name
            so_filename = self._pending_modules[module_name]

            del self._pending_modules[module_name]

            new_schema = RwYang.Model.load_and_merge_schema(self._schema, so_filename, module_name)
            self._schema = new_schema

            yang_model = RwYang.Model.create_libncx()
            yang_model.load_schema_ypbc(new_schema)    
           
            new_root = yang_model.get_root_node()

            self._schema_root = new_root
            self._confd_url_converter._schema = new_root
            self._xml_to_json_translator._schema = new_root
            self._schema_state = SchemaState.ready
            xact_info.respond_xpath(RwDts.XactRspCode.ACK, path.create_string())

        yield from self._dts.register(
                flags=RwDts.Flag.SUBSCRIBER,
                xpath="D,/rw-mgmt-schema:rw-mgmt-schema-state/rw-mgmt-schema:dynamic-modules",
                handler=rift.tasklets.DTS.RegistrationHandler(
                    on_prepare=load_modules_prepare))

        msg = RwRestconfYang.Restconfstats()

        def on_copy(shard, key, ctx):
            nonlocal msg
            msg.get_req = self._statistics.get_req
            msg.put_req = self._statistics.put_req
            msg.post_req = self._statistics.post_req
            msg.del_req = self._statistics.del_req
            msg.get_200_rsp = self._statistics.get_200_rsp
            msg.get_404_rsp = self._statistics.get_404_rsp
            msg.get_204_rsp = self._statistics.get_204_rsp
            msg.get_500_rsp = self._statistics.get_500_rsp
            msg.put_200_rsp = self._statistics.put_200_rsp
            msg.put_404_rsp = self._statistics.put_404_rsp
            msg.put_500_rsp = self._statistics.put_500_rsp
            msg.post_200_rsp = self._statistics.post_200_rsp
            msg.post_404_rsp = self._statistics.post_404_rsp
            msg.post_500_rsp = self._statistics.post_500_rsp
            msg.del_405_rsp = self._statistics.del_405_rsp
            msg.del_404_rsp = self._statistics.del_404_rsp
            msg.del_500_rsp = self._statistics.del_500_rsp
            msg.del_200_rsp = self._statistics.del_200_rsp
            msg.put_409_rsp = self._statistics.put_409_rsp
            msg.put_405_rsp = self._statistics.put_405_rsp
            msg.put_201_rsp = self._statistics.put_201_rsp
            msg.post_409_rsp = self._statistics.post_409_rsp
            msg.post_405_rsp = self._statistics.post_405_rsp
            msg.post_201_rsp = self._statistics.post_201_rsp
  
            return rwtypes.RwStatus.SUCCESS, msg.to_pbcm()

        @asyncio.coroutine
        def get_prepare(xact_info, action, ks_path, msg):
             xact_info.respond_xpath(rwdts.XactRspCode.NA, xpath="D,/rw-restconf:rwrestconf-statistics")
  
        reg = yield from self._dts.register(
                  flags=RwDts.Flag.PUBLISHER|RwDts.Flag.NO_PREP_READ,
                  xpath="D,/rw-restconf:rwrestconf-statistics",
                  handler=rift.tasklets.DTS.RegistrationHandler(on_prepare=get_prepare))

        shard = yield from reg.shard_init(flags=RwDts.Flag.PUBLISHER)
        shard.appdata_register_queue_key(copy=on_copy) 

        self._schema = RwRestconfYang.get_schema()
        
        self._dynamic_schema_registration = RwDynSchema.rwdynschema_instance_register(self._dts.handle, dyn_schema_callback, "RwRestconf", self)
    
    @asyncio.coroutine
    def run(self):
        self._schema_state = SchemaState.ready

    def _initialise_composite_and_start(self):
        for module_name, so_filename in self._pending_modules.items():
            new_schema = RwYang.Model.load_and_merge_schema(self._schema, so_filename, module_name)
            self._schema = new_schema

        yang_model = RwYang.Model.create_libncx()
        yang_model.load_schema_ypbc(self._schema)    
        self._schema_root = yang_model.get_root_node()

        self._netconf_connection_manager = ConnectionManager(self._log, self.loop, "127.0.0.1", "2022")
        self._confd_url_converter = ConfdRestTranslator(self._schema_root)
        self._xml_to_json_translator = XmlToJsonTranslator(self._schema_root)

        http_handler_arguments = {
            "logger" : self._log,
            "netconf_connection_manager" : self._netconf_connection_manager,
            "schema_root" : self._schema_root,
            "confd_url_converter" : self._confd_url_converter,
            "xml_to_json_translator" : self._xml_to_json_translator,
            "asyncio_loop" : self.loop,
            "configuration" : self._configuration,
            "statistics" : self._statistics,
        }
        application = tornado.web.Application([
            (r"/api/(.*)", HttpHandler, http_handler_arguments),
        ], compress_response=True)

        io_loop = rift.tasklets.tornado.TaskletAsyncIOLoop(asyncio_loop=self.loop)
        self._server = tornado.httpserver.HTTPServer(
                application,
                io_loop=io_loop,
                )

        self._server.listen("8888")
        
