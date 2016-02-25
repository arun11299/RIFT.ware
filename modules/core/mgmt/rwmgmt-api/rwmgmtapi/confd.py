
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import urllib3
import urllib3.util
import urllib3.exceptions
import urllib3.poolmanager
import lxml.etree
import rwmgmtapi.rwxml
import re
import rwmgmtapi.rest
import rwmgmtapi.app
import flask
import logging
import requests
from requests.auth import HTTPBasicAuth
import json

driver = None

class RestException(Exception):
    def __init__(self, status, message):
        self.status = status
        self.message = message

class Ns(object):
    QUERY_NS = "http://tail-f.com/ns/tailf-rest-query"
    QUERY = "{%s}" % QUERY_NS
    QUERY_PREFIX = 'q'

    REST_NS = "http://tail-f.com/ns/rest"
    REST = "{%s}" % REST_NS
    REST_PREFIX = 'y'

    NSMAP = {
        rwmgmtapi.rwxml.BASE_PREFIX : rwmgmtapi.rwxml.BASE_NS,
        rwmgmtapi.rwxml.FPATH_PREFIX : rwmgmtapi.rwxml.FPATH_NS,
        rwmgmtapi.rwxml.APPMGR_PREFIX : rwmgmtapi.rwxml.APPMGR_NS,
        rwmgmtapi.rwxml.IFMGR_PREFIX : rwmgmtapi.rwxml.IFMGR_NS,
        QUERY_PREFIX : QUERY_NS,
        REST_PREFIX : REST_NS
    }

class ContentType(object):
    YANG_COLLECTION_XML = 'application/vnd.yang.collection+xml'
    YANG_COLLECTION_JSON = 'application/vnd.yang.collection+json'
    YANG_DATA_XML = 'application/vnd.yang.data+xml'
    YANG_DATA_JSON = 'application/vnd.yang.data+json'

class QueryBuilder(rwmgmtapi.rwxml.Builder):
    def __init__(self):
        self.xml = self.e = lxml.etree.Element(Ns.QUERY + 'start-query', nsmap={None : Ns.QUERY_NS})

    def query(self, url):
        self.e.append(self.q_elem('foreach', url))

    def select(self, expression, result_type='string', label=None):
        s = self.append(self.q_elem('select'))
        s.append(self.q_elem('expression', expression))
        s.append(self.q_elem('result-type', result_type))
        if label is not None:
            s.append(self.q_elem('label', label))

    def q_elem(self, tag, txt=None):
        e = lxml.etree.Element(Ns.QUERY + tag, nsmap={None : Ns.QUERY_NS})
        if txt is not None:
            e.text = txt
        return e

class ConfDResponse(object):
    def __init__(self, request, response_headers, status, response_data):
         self.request = request
         self.raw_headers = response_headers
         self.headers = self.clean_headers(self.raw_headers)
         self.status = status
         self.data = response_data

    def clean_headers(self, raw_headers):
        cleaned = {}
        for h in ['content-type']:
            if h in raw_headers:
                cleaned[h] = raw_headers[h]
        return cleaned

class ConfDRequest(object):

    def __init__(self):
        self.incomingQueryString = None
        self.headers = {}
        self.requests_headers = {}
        self.requests_data = {}
        self.requests_params = {}
        self.requests_auth = {}
        global driver
        if driver is None:
            raise "Uninitialized. Call ConfDDriver.Initialize first"
        self.pool = driver.pool
        self.headers.update(driver.auth_headers)
        self.baseUrl = driver.baseApiUrl
        self.payload = None
        self.method = 'GET'
        self.cacheable_uris = [
            '/running/colony',
            '/operational/colony',
            '/operational/tasklet?deep',
            '/running/colony?select=trafsim-service/name',
            '/running/colony?select=name'
        ]
        self.cache = rwmgmtapi.app.cache
        # Only useful as True when in debugging scenarios
        self.cache_all = False
        self.requests_base_url = driver.requests_base_url
        self.requests_proxy = driver.requests_proxy
        self.requests_auth = driver.requests_auth

    def accept(self, mime_type):
        self.headers['ACCEPT'] = mime_type
        self.requests_headers['ACCEPT'] = mime_type

    @staticmethod
    def make_passthru(incoming_environ, incoming_headers=None):
        effective_headers = incoming_headers or incoming_environ
        r = ConfDRequest()
        if 'CONTENT_TYPE' in effective_headers:
            # Apparently this is what urllib3 expects, and not CONTENT_TYPE
            r.headers['Content-Type'] = effective_headers['CONTENT_TYPE']
            r.requests_headers['Content-Type'] = effective_headers['CONTENT_TYPE']

        accept = None
        if 'ACCEPT' in effective_headers:
            accept = effective_headers['ACCEPT']
        elif 'HTTP_ACCEPT' in effective_headers:
            accept = effective_headers['HTTP_ACCEPT']

        if accept is None or accept == 'application/json' or accept == '*/*':
            accept = ContentType.YANG_DATA_JSON

        r.headers['Accept'] = accept
        r.requests_headers['Accept'] = accept

        if 'QUERY_STRING' in effective_headers:
            q = effective_headers['QUERY_STRING']
            if len(q) > 0:
                r.incomingQueryString = q
                r.requests_params = q

        if 'REQUEST_METHOD' in incoming_environ:
            r.method = incoming_environ['REQUEST_METHOD']
            if r.method == 'POST' or r.method == 'PUT':
                if 'wsgi.input' in effective_headers:
                    r.payload = effective_headers['wsgi.input'].read()
                    r.requests_data = r.payload

        return r

    def set_payload(self, payload):
        if self.method == 'GET':
            self.method = 'POST'
        self.payload = payload

    def send(self, url):
        # Go thru cache as some calls are expensive and content does not change.
        cache_id = self.make_cache_id(url)
        if cache_id is not None:
            with rwmgmtapi.app.app.app_context():
                cache_hit = self.cache.get(cache_id)
                if cache_hit is not None:
                    logging.debug('Cache hit:' + url)
                    return (cache_hit[0], 200, cache_hit[1])
        try:
            response = requests.request(self.method, self.requests_base_url + url, headers=self.requests_headers, data=self.requests_data, params=self.requests_params, proxies=self.requests_proxy, auth=self.requests_auth)

            #response = self.pool.urlopen(self.method, self.baseUrl + url, body=self.payload, headers=self.headers)
            #response_headers = response.getheaders()
            # if cache_id is not None and response.status == 200:
            #     with rwmgmtapi.app.app.app_context():
            #         # As of 2/18/15 we do not use headers and therefore not a strict requirement
            #         # to cache them, however having response w/o headers would be fairly limiting
            #         # going forward
            #         self.cache.add(cache_id, (response.data, response_headers))
            # return (response.data, response.status, response_headers)
            return (response.content, response.status_code, response.headers)
        except urllib3.exceptions.MaxRetryError as noConnection:
            raise rwmgmtapi.rest.RestException(500, 'No response from ConfD server')

    def get_accept(self):
        for h in ['ACCEPT', 'Accept']:
            if h in self.headers:
                return self.headers[h]
        return None

    def make_cache_id(self, url):
        if self.method == 'GET':
            fqurl = url
            if self.incomingQueryString is not None:
                fqurl += '?' + self.incomingQueryString
            if self.cache_all or fqurl in self.cacheable_uris:
                return self.get_accept() + '~' + fqurl

        return None

    def check_cache(self, url):
        return None

    def flask_send(self, url):
        data, status, headers = self.send(url)
        ConfDRequest.check_response(status, data)
        return (data, status, headers)

    def flask_require(self, url):
        data, status, _ = self.flask_send(url)
        if status == 204 or len(data) == 0:
            raise rwmgmtapi.rest.RestException(500, 'Empty response from ' + url)
        return data

    @staticmethod
    def check_response(status_code, content):
        if status_code < 400:
            return

        content = 'error'
        para_content_re = re.compile('<p>(.*)</p>')
        found_content = para_content_re.findall(content)
        if len(found_content) > 0:
            content = found_content[0]
        raise rwmgmtapi.rest.RestException(status_code, content)

# ConfD REST requests helper
class ConfDDriver(object):
    def __init__(self, server, username, password, proxy):
        if proxy == None:
            # RIFT-6957 - Douglas hit limit at 10, increasing to 30
            self.pool = urllib3.HTTPConnectionPool(server, maxsize=30)
            self.baseApiUrl = '/api'
            self.requests_base_url = 'http://' + server + '/api'
            self.requests_proxy = {}
        else:
            # RIFT-6957 - Douglas hit limit at 10, increasing to 30
            self.pool = urllib3.poolmanager.ProxyManager(proxy, num_pools=30)
            self.baseApiUrl = 'http://' + server + '/api'
            self.requests_base_url = 'http://' + server + '/api'
            self.requests_proxy = {
                "http": proxy
            }
        self.auth_headers = urllib3.util.make_headers(basic_auth = username + ':' + password, keep_alive = True)
        self.requests_auth = HTTPBasicAuth(username, password)

    @staticmethod
    def Initialize(server, username, password, proxy=None):
        global driver
        driver = ConfDDriver(server, username, password, proxy)

# URL that match certain pattern go straight to ConfD's REST server
class ConfDPassThru(object):

    def __init__(self, app):
        self.app = app
        methods = ['GET', 'POST', 'PUT', 'DELETE', 'PATCH']
        self.app.add_url_rule('/api/<path:path>', __name__, self.passthru, methods=methods)

    def passthru(self, path):
        url = '/' + path
        req = ConfDRequest.make_passthru(flask.request.environ)
        # if req.incomingQueryString is not None:
        #     url += '?' + req.incomingQueryString
        data, status, headers = req.send(url)
        return flask.Response(response=data, status=status, headers={})
