
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import flask
from rwmgmtapi.app import app
import rwmgmtapi.stats
import wsgiref.util

stats = rwmgmtapi.stats.SessionAwareStats()

@app.before_request
def start_timer():
    global stats
    module_id = wsgiref.util.request_uri(flask.request.environ, include_query=1)
    flask.request.perf_token = stats.record_start(flask.request.headers, module_id)

@app.after_request
def stop_timer(response):
    stats.record_end(flask.request.environ, flask.request.perf_token)
    return response
