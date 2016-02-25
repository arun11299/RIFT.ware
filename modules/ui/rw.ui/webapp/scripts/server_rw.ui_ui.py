#!/usr/bin/env python3

# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import http.server
import socketserver
import mimetypes

PORT = 8000

Handler = http.server.SimpleHTTPRequestHandler

Handler.extensions_map['.svg']='image/svg+xml'
httpd = socketserver.TCPServer(("", PORT), Handler)

print("serving at port: {}".format(PORT))
httpd.serve_forever()
