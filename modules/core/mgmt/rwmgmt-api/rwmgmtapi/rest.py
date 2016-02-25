
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import rwmgmtapi.xml2json
import re
import flask

ERR_XML_SCHEMA = rwmgmtapi.xml2json.XmlDecoderStrategy()


class RestException(Exception):
    def __init__(self, status, message):
        Exception.__init__(self)
        self.status = status
        self.message = message
        if flask.request and 'Accept' in flask.request.headers:
            self.accept = flask.request.headers['Accept']
        else:
            self.accept = 'application/json'

    def body(self):
        err = '<error><status>%d</status><message>%s</message></error>'
        error_xml = err % (self.status, self.message)
        is_json = re.compile('^.*[+/]json$')
        if is_json.match(self.accept) != None:
            return rwmgmtapi.rwxml.xml_to_json(error_xml, ERR_XML_SCHEMA)
        return error_xml
