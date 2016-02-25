
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

from rwmgmtapi.app import app, cache
from rwmgmtapi.model import browser
import json

@app.route('/model/config/<base>')
@cache.cached()
def model(base):
    s = browser.Schema(keep_empty_collections=False)
    b = browser.Browser(base)
    b.walk(s, browser.ConfigOnly())
    return json.dumps(s.root)
