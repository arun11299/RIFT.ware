# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Max Beckett
# Creation Date: 7/10/2015
# 

import asyncio
import lxml.etree 
import time

import ncclient
import tornado.gen
import tornado.httpserver
import tornado.ioloop
import tornado.platform.asyncio
import tornado.web
import tornado.websocket

from ..translation import (
    convert_netconf_response_to_json,
    convert_rpc_to_json_output,
    convert_rpc_to_xml_output,
    convert_xml_to_collection,
)
from ..util import (
    create_xpath_from_url,
    is_config
)
from .netconf_wrapper import Result

class HttpHandler(tornado.web.RequestHandler):
    def initialize(self, logger, netconf_connection_manager, schema_root, confd_url_converter, xml_to_json_translator, asyncio_loop, configuration, statistics):
        # ATTN: can't call it _log becuase the base class has a _log instance already
        self.__log = logger
        self._asyncio_loop = asyncio_loop
        self._base_error_message = base_message = "<html><body>%s</body></html>"
        self._confd_url_converter = confd_url_converter
        self._schema_root = schema_root
        self._xml_to_json_translator = xml_to_json_translator
        self._configuration = configuration
        self._default_accept_type = "application/vnd.yang.data+xml"
        self._statistics = statistics
        self._netconf_connection_manager = netconf_connection_manager

    def _get_encoded_username_and_password(self):
        auth_header = self.request.headers.get("Authorization")        
        if auth_header is None:
            raise ValueError("no authorization header present")
        if not auth_header.startswith("Basic "):
            raise ValueError("only supoprt for basic authorization")

        return auth_header[6:]

    @asyncio.coroutine
    def connect(self):
        ''' must be called from inside a coroutine impl '''
        self._netconf = yield from self._netconf_connection_manager.get_connection(self._get_encoded_username_and_password())

    @asyncio.coroutine
    def _handle_upgrade(self):
        ''' must be called from inside a coroutine impl '''
        yield from self._netconf_connection_manager.reconnect(self._get_encoded_username_and_password())

    @tornado.gen.coroutine
    def delete(self, *args, **kwargs):
        self._statistics._del_req += 1
        if self._configuration.log_timing:        
            start_time = time.clock_gettime(time.CLOCK_REALTIME)
            #self.__log.debug("get start time: %s " % start_time)
            
        @asyncio.coroutine
        def impl():
            try:
                yield from self.connect()
            except ncclient.transport.errors.AuthenticationError as e:
                return 401, "text/html", "bad username/password"
            except ValueError as e:
                return 401, "text/html", "must supply BasicAuth"

            url = self.request.uri
            #self.__log.debug("DELETE: %s" % (url))

            accept_type = self.request.headers.get("Accept")
            if accept_type is None:
                accept_type = self._default_accept_type
            is_collection = "collection" in accept_type

            if not is_config(url):
                self._statistics._del_405_rsp += 1
                return 405, "text/html", "can't delete operational data"

            try:
                converted_url = self._confd_url_converter.convert("DELETE", url, None)
            except Exception as e:
                #self.__log.debug("invalid url requested %s" % url)
                self._statistics._del_404_rsp += 1
                return 404, "text/html", self._base_error_message % "Resource target or resource node not found"

            #self.__log.debug("converted url: %s" % converted_url)

            if self._configuration.log_timing:
                netconf_start_time = time.clock_gettime(time.CLOCK_REALTIME)
                #self.__log.debug("delete netconf start time: %s " % netconf_start_time)

            result, netconf_response = yield from self._netconf.delete(converted_url)
                    
            if self._configuration.log_timing:
                netconf_end_time = time.clock_gettime(time.CLOCK_REALTIME)
                #self.__log.debug("delete netconf end time: %s " % netconf_end_time)

            if result != Result.OK:
                if result == Result.Upgrade_Performed:
                    yield from self._netconf.connect()
                    return_code = 500
                    self._statistics._get_500_rsp += 1
                elif result == Result.Unknown_Error:
                    return_code = 500
                    self._statistics._get_500_rsp += 1
                elif result == Result.Upgrade_In_Progress:
                    return_code = 405
                elif result == Result.Operation_Failed:
                    return_code = 405
                else:
                    return_code = 500

            if "data-missing" in netconf_response:
                self._statistics.del_404_rsp += 1
                return_code = 404
            else:
                self._statistics.del_200_rsp += 1
                return_code = 200

            if "json" in accept_type:
                response = convert_netconf_response_to_json(bytes(netconf_response,"utf-8"))
                #self.__log.debug("converted json: %s" % response)
                return return_code, accept_type, response

            return return_code, accept_type, netconf_response

        f = asyncio.async(impl(), loop=self._asyncio_loop)
        response = yield tornado.platform.asyncio.to_tornado_future(f)
        
        http_code, content_type, message = response[:]
        self.clear()
        self.set_status(http_code)
        self.set_header("Content-Type", content_type)
        self.write(message)
        if self._configuration.log_timing:
            end_time = time.clock_gettime(time.CLOCK_REALTIME)
            #self.__log.debug("delete end time: %s " % end_time)

    @tornado.gen.coroutine
    def get(self, *args, **kwargs):
        self._statistics._get_req += 1
        if self._configuration.log_timing:        
            start_time = time.clock_gettime(time.CLOCK_REALTIME)
            #self.__log.debug("get start time: %s " % start_time)
            
        @asyncio.coroutine
        def impl():
            try:
                yield from self.connect()
            except ncclient.transport.errors.AuthenticationError as e:
                return 401, "text/html", "bad username/password"
            except ValueError as e:
                return 401, "text/html", "must supply BasicAuth"

            url = self.request.uri
            accept_type = self.request.headers.get("Accept")

            if accept_type is None:
                accept_type = self._default_accept_type

            is_collection = "collection" in accept_type
                
            #self.__log.debug("GET: %s %s" % (url, accept_type))

            try:
                converted_url = self._confd_url_converter.convert("GET", url, None)
            except Exception as e:
                #self.__log.debug("invalid url requested %s" % url)
                self._statistics._get_404_rsp += 1
                return 404, "text/html", self._base_error_message % "Resource target or resource node not found"

            #self.__log.debug("converted url: %s" % converted_url)

            if self._configuration.log_timing:
                netconf_start_time = time.clock_gettime(time.CLOCK_REALTIME)
                #self.__log.debug("get netconf start time: %s " % netconf_start_time)

            if is_config(url):
                result, netconf_response = yield from self._netconf.get_config(converted_url)
            else:
                result, netconf_response = yield from self._netconf.get(converted_url)

            if self._configuration.log_timing:
                netconf_end_time = time.clock_gettime(time.CLOCK_REALTIME)
                #self.__log.debug("get netconf end time: %s " % netconf_end_time)

            if result != Result.OK:
                if result == Result.Upgrade_Performed:
                    yield from self._netconf.connect()
                    return_code = 500
                    self._statistics._get_500_rsp += 1
                elif result == Result.Unknown_Error:
                    return_code = 500
                    self._statistics._get_500_rsp += 1
                elif result == Result.Upgrade_In_Progress:
                    return_code = 405
                elif result == Result.Operation_Failed:
                    return_code = 405
                else:
                    return_code = 500

                return return_code, "application/vnd.yang.data+xml", netconf_response

            if "json" not in accept_type:
                if is_collection:
                    response = convert_xml_to_collection(url, netconf_response)
                    #self.__log.debug("collection xml: %s" % response)
                else:
                    # strip off <data></data> tags

                    root = lxml.etree.fromstring(netconf_response)
                    try:
                        lxml.etree.tostring(root[0])
                    except:
                        #self.__log.debug("empty response from confd")
                        self._statistics._get_204_rsp += 1
                        return 204, "", ""
                    response = netconf_response
            else:
                xpath = create_xpath_from_url(url, self._schema_root)

                try:
                    # check for no content
                    root = lxml.etree.fromstring(netconf_response)
                    lxml.etree.tostring(root[0])
                except:
                    #self.__log.debug("empty response from confd")
                    self._statistics._get_204_rsp += 1
                    return 204, "", ""

                try:
                    response = self._xml_to_json_translator.convert(is_collection, url, xpath, netconf_response)
                    #self.__log.debug("converted json: %s" % response)
                except IndexError:
                    self._statistics._get_204_rsp += 1
                    return 204, "", ""
                except Exception as e:
                    #self.__log.debug("malformed response from confd: %s %s" % (netconf_response,e))
                    self._statistics._get_500_rsp += 1
                    return 500, "application/vnd.yang.data+xml", str(netconf_response)

            self._statistics._get_200_rsp += 1
            return 200, accept_type, response

        f = asyncio.async(impl(), loop=self._asyncio_loop)
        response = yield tornado.platform.asyncio.to_tornado_future(f)
        
        http_code, content_type, message = response[:]
        self.clear()

        self.set_status(http_code)
        if http_code != 204:
            self.set_header("Content-Type", content_type)
            self.write(message)

        if self._configuration.log_timing:
            end_time = time.clock_gettime(time.CLOCK_REALTIME)
            #self.__log.debug("get end time: %s " % end_time)
            

    @tornado.gen.coroutine
    def put(self, *args, **kwargs):
        self._statistics._put_req += 1
        if self._configuration.log_timing:
            start_time = time.clock_gettime(time.CLOCK_REALTIME)
            #self.__log.debug("put start time: %s " % start_time)

        @asyncio.coroutine
        def impl():
            try:
                yield from self.connect()
            except ncclient.transport.errors.AuthenticationError as e:
                return 401, "text/html", "bad username/password"
            except ValueError as e:
                return 401, "text/html", "must supply BasicAuth"

            url = self.request.uri
            #self.__log.debug("PUT: %s" % url)

            body = self.request.body.decode("utf-8")
            body_header = self.request.headers.get("Content-Type")

            try:
                converted_url = self._confd_url_converter.convert("PUT", url, (body,body_header))
            except:
                #self.__log.debug("invalid url requested %s" % url)
                self._statistics._put_404_rsp += 1
                return 404, "text/html", self._base_error_message % "Resource target or resource node not found"

            #self.__log.debug("converted url: %s" % converted_url)

            if self._configuration.log_timing:
                netconf_start_time = time.clock_gettime(time.CLOCK_REALTIME)
                #self.__log.debug("put netconf start time: %s " % netconf_start_time)


            result, netconf_response =  yield from self._netconf.put(converted_url)

            if self._configuration.log_timing:
                netconf_end_time = time.clock_gettime(time.CLOCK_REALTIME)
                #self.__log.debug("put netconf end time: %s " % netconf_end_time)

            if result != Result.OK:
                if result == Result.Upgrade_Performed:
                    yield from self._netconf.connect()
                    return_code = 500
                    self._statistics._get_500_rsp += 1
                elif result == Result.Unknown_Error:
                    return_code = 500
                    self._statistics._get_500_rsp += 1
                elif result == Result.Upgrade_In_Progress:
                    return_code = 405
                elif result == Result.Operation_Failed:
                    return_code = 405
                elif result == Result.Data_Exists:
                    return_code = 409
                    self._statistics._put_409_rsp += 1
                elif result == Result.Rpc_Error:
                    self._statistics._put_405_rsp += 1
                    return_code = 405
                else:
                    return_code = 500
            else:
                return_code = 201
                self._statistics._put_201_rsp += 1
                
            accept_type = self.request.headers.get("Accept")
            if accept_type is None:
                accept_type = self._default_accept_type

            is_collection = "collection" in accept_type

            if "json" in accept_type:
                response = convert_netconf_response_to_json(bytes(netconf_response,"utf-8"))
                #self.__log.debug("converted json: %s" % response)
                return return_code, accept_type, response

            return return_code, accept_type, netconf_response

        f = asyncio.async(impl(), loop=self._asyncio_loop)
        response = yield tornado.platform.asyncio.to_tornado_future(f)

        http_code, content_type, message = response[:]
        self.clear()
        self.set_status(http_code)
        self.set_header("Content-Type", content_type)
        self.write(message)

        if self._configuration.log_timing:
            end_time = time.clock_gettime(time.CLOCK_REALTIME)
            #self.__log.debug("put end time: %s " % end_time)

    @tornado.gen.coroutine
    def post(self, *args, **kwargs):
        self._statistics._post_req += 1
        if self._configuration.log_timing:
            start_time = time.clock_gettime(time.CLOCK_REALTIME)
            #self.__log.debug("post start time: %s " % start_time)

        @asyncio.coroutine
        def impl():
            try:
                yield from self.connect()
            except ncclient.transport.errors.AuthenticationError as e:
                return 401, "text/html", "bad username/password"
            except ValueError as e:
                return 401, "text/html", "must supply BasicAuth"

            url = self.request.uri
            #self.__log.debug("POST: %s" % url)

            body = self.request.body.decode("utf-8")
            body_header = self.request.headers.get("Content-Type")

            converted_url = self._confd_url_converter.convert("POST", url, (body, body_header))
            try:
                pass
            except:
                #self.__log.debug("invalid url requested %s" % url)
                self._statistics._post_404_rsp += 1
                return 404, "text/html", self._base_error_message % "Resource target or resource node not found"

            is_operation = "/api/operations" in url

            #self.__log.debug("converted url: %s" % converted_url)

            if self._configuration.log_timing:
                netconf_start_time = time.clock_gettime(time.CLOCK_REALTIME)
                #self.__log.debug("post netconf start time: %s " % netconf_start_time)

            result, netconf_response = yield from self._netconf.post(converted_url, is_operation)

            if self._configuration.log_timing:
                netconf_end_time = time.clock_gettime(time.CLOCK_REALTIME)
                #self.__log.debug("post netconf end time: %s " % netconf_end_time)

            if result != Result.OK:
                if result == Result.Upgrade_Performed:
                    yield from self._netconf.connect()
                    return_code = 500
                    self._statistics._get_500_rsp += 1
                elif result == Result.Unknown_Error:
                    return_code = 500
                    self._statistics._get_500_rsp += 1
                elif result == Result.Upgrade_In_Progress:
                    return_code = 405
                elif result == Result.Operation_Failed:
                    #self.__log.debug("operation failed")
                    return_code = 405
                elif result == Result.Data_Exists:
                    return_code = 409
                    self._statistics._put_409_rsp += 1
                elif result == Result.Rpc_Error:
                    #self.__log.debug("rpc error")
                    self._statistics._put_405_rsp += 1
                    return_code = 405
                else:
                    return_code = 500
            else:
                return_code = 201
                self._statistics._put_201_rsp += 1

            accept_type = self.request.headers.get("Accept")
            if accept_type is None:
                accept_type = self._default_accept_type

            is_collection = "collection" in accept_type

            if "json" in accept_type:
                response = convert_netconf_response_to_json(bytes(netconf_response,"utf-8"))
                #self.__log.debug("converted json: %s" % response)
            else:
                response = netconf_response
                
            if is_operation:
                if "json" in accept_type:
                    response = convert_rpc_to_json_output(response)
                else:
                    response = convert_rpc_to_xml_output(response)

            return return_code, accept_type, response

        f = asyncio.async(impl(), loop=self._asyncio_loop)
        response = yield tornado.platform.asyncio.to_tornado_future(f)

        http_code, content_type, message = response[:]
        self.clear()
        self.set_status(http_code)
        self.set_header("Content-Type", content_type)
        self.write(message)

        if self._configuration.log_timing:
            end_time = time.clock_gettime(time.CLOCK_REALTIME)
            #self.__log.debug("post end time: %s " % end_time)
