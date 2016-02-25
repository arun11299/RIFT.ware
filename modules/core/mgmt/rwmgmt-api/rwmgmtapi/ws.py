
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import socket
from rwmgmtapi.app import websocket
import flask
import time
import thread
import requests
import rwmgmtapi.xml2json
import logging
import rwmgmtapi.rest

@websocket.on('connect', namespace='/rwapp')
def rwapp_connect():
    logging.info("connect called")
    flask.request.namespace.poller = Poller()
    #request.namespace.poller.gs = gevent.spawn(request.namespace.poller.poll)
    flask.request.namespace.poller.thread = thread.start_new_thread(flask.request.namespace.poller.poll)

class Poller:
    def __init__(self):
        self.workers = {}

    def worker(self, id, desc, worker=None):
        self.workers[id] = { 'desc': desc, 'func': worker}
        logging.info("adding/updating %d workers" % len(self.workers))

    def removeWorker(self, id):
        if id in self.workers:
            del self.workers[id]
            #self.workers.pop(id)

    def poll(self):
        self.working = True
        while self.working:
            # sleep first so client doesn't get data before it's ready
            time.sleep(2)
            #gevent.sleep(2)
            logging.info("doing work on %d workers" % len(self.workers))
            # copy to keep threadsafe
            w = self.workers.copy()
            for workerId in w.keys():
                try:
                    logging.info("starting work")
                    w[workerId]['func']()
                    logging.info("finished work")
                except Exception as e:
                    logging.info(str(e))
        logging.info("stopped thread")

@websocket.on('disconnect', namespace='/rwapp')
def rwapp_disconnect():
    if hasattr(flask.request.namespace, 'poller'):
        flask.request.namespace.poller.working = False

@websocket.on('web/get', namespace='/rwapp')
def ws_web_get(meta):
    if not poller_init(meta):
        return
    @flask.copy_current_request_context
    def run():
        # TODO: copy appropriate ws headers into http headers (like auth)
        headers={}
        accept = 'application/json'
        if 'accept' in meta:
            accept = meta['accept']
        headers['Accept'] = accept
        url = chomp_fwdslash(flask.request.host_url) +  meta['url']
        method = 'GET'
        if 'method' in meta:
            method = meta['method'].upper()
        if method == 'POST':
            payload = meta['data']
            content_type = 'application/vnd.yang.operation+json'
            if 'contentType' in meta:
                content_type = meta['contentType']
            headers['Content-Type'] = content_type
            response = requests.post(url, payload, headers=headers)
        else:
            response = requests.get(url, headers=headers)
        try:
            rwmgmtapi.confd.ConfDRequest.check_response(response.status_code, response.content)
        except rwmgmtapi.rest.RestException as e:
            # response error message and code is already in payload
            e.accept = accept
            response.content = e.body()

        flask.ext.socketio.emit('web/get/%s' % meta['widgetId'], response.content)
    poller_finish(meta, meta['url'], run)

def chomp_fwdslash(url):
    if url[-1] == '/':
        return url[:-1]
    return url

def poller_init(meta):
    logging.info(meta)
    if 'enable' in meta and not meta['enable']:
        flask.request.namespace.poller.removeWorker(meta['widgetId'])
        return False
    return True

def poller_finish(meta, desc, func):
    try:
        logging.info("starting get w/request")
        flask.request.namespace.poller.worker(meta['widgetId'], desc, func)
    except socket.error as e:
        flask.ext.socketio.emit('alert', 'Could not connect to netconf server.'
            + e.strerror)
