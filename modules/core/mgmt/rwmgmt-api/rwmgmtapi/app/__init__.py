
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

from flask import Flask
from flask.ext.socketio import SocketIO
import logging
from rwmgmtapi.app.cache import Cache

app = None
websocket = None
cache = None

def create_app(ui_dir):
    global app
    app = Flask(__name__, static_folder=ui_dir, static_url_path='')
    app.config['SECRET_KEY'] = 'secret!'
    cache = create_cache()
    cache.init_app(app)
    return app

def create_websocket():
    global app
    global websocket
    websocket = SocketIO(app)
    return websocket

def create_cache():
    global cache
    logging.info("Cache created")
    cache = Cache(config={'CACHE_TYPE': 'simple'})
    return cache
