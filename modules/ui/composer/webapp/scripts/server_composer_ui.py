#!/usr/bin/python

# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import SimpleHTTPServer
import SocketServer
import mimetypes

PORT = 9000

Handler = SimpleHTTPServer.SimpleHTTPRequestHandler

Handler.extensions_map['.svg']='image/svg+xml'
httpd = SocketServer.TCPServer(("", PORT), Handler)

print "serving at port", PORT
httpd.serve_forever()
