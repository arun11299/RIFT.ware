
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import lxml.etree

from rwmgmtapi.app import app, cache
import rwmgmtapi.rwxml
import rwmgmtapi.vnf

@app.route('/vnf/')
@cache.cached()
def vnf():
    xml = rwmgmtapi.vnf.Service().vnfs()
    return rwmgmtapi.rwxml.xml_content(lxml.etree.tostring(xml), rwmgmtapi.rwxml.CONFIG_SCHEMA)
