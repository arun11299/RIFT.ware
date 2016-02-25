# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Creation Date: 10/6/2015

# 

import asyncio
import base64
from enum import Enum
import lxml

import ncclient.asyncio_manager

NCCLIENT_WORKER_COUNT = 5

class Result(Enum):
    OK = 1
    Upgrade_In_Progress = 2
    Upgrade_Performed = 3
    Operation_Failed = 4
    Rpc_Error = 5
    Data_Exists = 6
    Unknown_Error = 7
    Commit_Failed = 8

def _check_exception(error_text):
    if ("session close" in error_text) or ("Not connected" in error_text) or ("Capabilities changed" in error_text) or ("Not connected to NETCONF server" in error_text):
        result = Result.Upgrade_Performed
    else:
        result = Result.Unknown_Error
    return result

def _check_netconf_response(netconf_response, commit_response):
    if netconf_response.ok:
        netconf_result = Result.OK
        if "data-exists" in netconf_response.xml:
            netconf_result = Result.Data_Exists
        elif "rpc-error" in netconf_response.xml:
            netconf_result = Result.Rpc_Error
    else:
        if "resource-denied" in netconf_response.error.tag:
            return Result.Upgrade_In_Progress
        else:
            return Result.Operation_Failed

    if commit_response is not None and "rpc-error" in commit_response.xml:
        return Result.Commit_Failed
    else:
        return netconf_result

class ConnectionManager(object):
    def __init__(self, log, loop, netconf_ip, netconf_port):
        self._log = log
        self._loop = loop
        self._netconf_ip = netconf_ip 
        self._netconf_port = netconf_port
        self._connections = {}

    @asyncio.coroutine
    def get_connection(self, encoded_username_and_password):
        auth_decoded = base64.decodestring(bytes(encoded_username_and_password, "utf-8")).decode("utf-8")
        username, password = auth_decoded.split(":",2)

        if encoded_username_and_password in self._connections.keys():
            index, connections = self._connections[encoded_username_and_password][:]
            ret = connections[index]
            index += 1

            if index >= len(connections):
                index = 0

            self._connections[encoded_username_and_password] = (index, connections)
                
            return ret

        wrappers = list()
        
        #self._log.debug("starting new connection with confd for %s" % username)
        for _ in range(NCCLIENT_WORKER_COUNT):
            new_wrapper = NetconfWrapper(self._log, self._loop, self._netconf_ip, self._netconf_port, username, password)
            yield from new_wrapper.connect()

            wrappers.append(new_wrapper)

        self._connections[encoded_username_and_password] = (0, wrappers)

        return wrappers[0]

    @asyncio.coroutine
    def reconnect(self, encoded_username_and_password):
        for itr in range(NCCLIENT_WORKER_COUNT):
            yield from self._connections[encoded_username_and_password][1][itr].connect()

class NetconfWrapper(object):

    def __init__(self, logger, loop, netconf_ip, netconf_port, username, password):
        self._log = logger
        self._loop = loop
        self._netconf_ip = netconf_ip 
        self._netconf_port = netconf_port
        self._username = username
        self._password = password

    @asyncio.coroutine
    def connect(self):
        while True:
            try:
                self._netconf = yield from ncclient.asyncio_manager.asyncio_connect(
                    loop=self._loop,
                    host=self._netconf_ip,
                    port=self._netconf_port,
                    username=self._username,
                    password=self._password,
                    allow_agent=False,
                    look_for_keys=False,
                    hostkey_verify=False
                )
                #self._log.info("Connected to confd")
                break
            except ncclient.transport.errors.SSHError as e:
                #self._log.error("Failed to connect to confd")
                yield from asyncio.sleep(2, loop=self._loop)

    @asyncio.coroutine
    def get(self, xml):
        if not self._netconf.connected:
            yield from self.connect()

        try:
            netconf_response = yield from self._netconf.get(('subtree',xml))
        except Exception as e:
            #self._log.error("ncclient query failed: %s" % e)
            error_text = str(e)
            error_code = _check_exception(error_text)
            return error_code, error_text

        #self._log.debug("netconf get response: %s", netconf_response.xml)
        result = _check_netconf_response(netconf_response, None)

        if result == Result.OK:
            response = netconf_response.data_xml
        else:
            response = netconf_response.xml

        return result, response

    @asyncio.coroutine
    def get_config(self, xml):
        if not self._netconf.connected:
            yield from self.connect()

        try:
            netconf_response = yield from self._netconf.get_config(
                source="running",
                filter=('subtree',xml))
        except Exception as e:
            #self._log.error("ncclient query failed: %s" % e)
            error_text = str(e)
            error_code = _check_exception(error_text)
            return error_code, error_text

        #self._log.debug("netconf get config response: %s", netconf_response.xml)
        result = _check_netconf_response(netconf_response, None)

        if result == Result.OK:
            response = netconf_response.data_xml
        else:
            response = netconf_response.xml

        return result, response

    @asyncio.coroutine
    def delete(self, xml):
        if not self._netconf.connected:
            yield from self.connect()

        netconf_response = yield from self._netconf.edit_config(
            target="running",
            config=xml)

        #self._log.debug("netconf delete response: %s", netconf_response.xml)

        # ATTN: uncomment when candidate is the target
        commit_response = None
        #commit_response = yield from self._netconf.commit()
        #self._log.debug("netconf delete commit netconf_response: %s", commit_response.xml)

        result = _check_netconf_response(netconf_response, None)

        return result, netconf_response.xml

    @asyncio.coroutine
    def put(self, xml):
        if not self._netconf.connected:
            yield from self.connect()

        netconf_response = yield from self._netconf.edit_config(
            target="running",
            config=xml)
        #self._log.debug("netconf put response: %s", netconf_response.xml)

        # ATTN: uncomment when candidate is the target
        commit_response = None
        #commit_response = yield from self._netconf.commit()
        #self._log.debug("netconf put commit netconf_response: %s", commit_response.xml)

        result = _check_netconf_response(netconf_response, None)

        return result, netconf_response.xml
            
    @asyncio.coroutine
    def post(self, xml, is_operation):
        if not self._netconf.connected:
            yield from self.connect()

        if is_operation:
            netconf_response = yield from self._netconf.dispatch(lxml.etree.fromstring(xml))
            #self._log.debug("netconf post-rpc response: %s", netconf_response.xml)
            commit_response = None
        else:
            netconf_response = yield from self._netconf.edit_config(
                target="running",
                config=xml)

            # ATTN: uncomment when candidate is the target
            commit_response = None
            #commit_response = yield from self._netconf.commit()
            #self._log.debug("netconf post-config commit netconf_response: %s", commit_response.xml)

            #self._log.debug("netconf post-config response: %s", netconf_response.xml)

        result = _check_netconf_response(netconf_response, None)

        return result, netconf_response.xml

