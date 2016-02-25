
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import lxml.etree
import rwmgmtapi.rwxml
from rwmgmtapi.app import app, cache
import rwmgmtapi.rwxml
import rwmgmtapi.vcs

@app.route('/vcs/', methods=['GET'])
@cache.cached()
def vcs():
    s = rwmgmtapi.rwxml.remove_ns(rwmgmtapi.vcs.Service().vcs())
    return rwmgmtapi.rwxml.xml_content(lxml.etree.tostring(s), rwmgmtapi.rwxml.CONFIG_SCHEMA)
