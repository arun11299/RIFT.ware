
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

# Avoids benign error
#   http://stackoverflow.com/questions/8774958/keyerror-in-module-threading-after-a-successful-py-test-run
#
import gevent.monkey; gevent.monkey.patch_thread()
import threading

from lxml.etree import XML
from jinja2 import Environment, PackageLoader
from rwmgmtapi.app import create_test_app, create_test_websocket

create_test_app()
create_test_websocket()

env = Environment(loader=PackageLoader('rwmgmtapi', '.'))

def load_test_xml(fname, args={}):
    template = env.get_template(fname)
    args['ns_base'] = 'xmlns:rw-base="http://riftio.com/ns/riftware-1.0/rw-base"'
    args['ns_appmgr'] = 'xmlns:rwappmgr="http://riftio.com/ns/riftware-1.0/rwappmgr"'
    return XML(template.render(args))
