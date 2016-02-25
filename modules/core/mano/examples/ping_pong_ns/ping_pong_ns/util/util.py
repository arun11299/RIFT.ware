# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

from datetime import date
import urlparse

import tornado.web

class VersionHandler(tornado.web.RequestHandler):
    def initialize(self, instance):
        self._instance = instance

    def get(self):
        response = { 'version': '3.5.1',
                     'last_build':  date.today().isoformat() }
        self.write(response)
 
def get_url_target(url):
    is_operation = False
    url_parts = urlparse.urlsplit(url)
    whole_url = url_parts[2]

    url_pieces = whole_url.split("/")
    
    return url_pieces[-1]
