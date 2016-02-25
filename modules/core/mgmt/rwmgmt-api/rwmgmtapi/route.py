
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

# Loading these packages registers all the handlers
import fpath.route
import vcs.route
import vnf.route
import ws
import internal
import artifacts
from rwmgmtapi.app import app
import rwmgmtapi.rest
import rwmgmtapi.web_stats

@app.errorhandler(rwmgmtapi.rest.RestException)
def rest_error(e):
    return e.body(), e.status
