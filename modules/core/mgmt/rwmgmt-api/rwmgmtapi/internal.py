
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

from rwmgmtapi.app import app, websocket
import flask.ext.socketio
import flask
import json
import csv
import rwmgmtapi.ws
from rwmgmtapi.web_stats import stats as wsgi_stats

OK = '{"ok":""}'

@websocket.on('internal/stats', namespace='/rwapp')
def ws_internal_stats(meta):
    if not rwmgmtapi.ws.poller_init(meta):
        return
    @flask.copy_current_request_context
    def run():
        worker_urls = {}
        workers = flask.request.namespace.poller.workers
        for worker_id, worker in workers.items():
            count(worker_urls, worker['desc'])
        stats = server_stats()
        stats['clientWebsocket'] = {
            'workers' : len(workers),
            'subscriptions' : worker_urls
        }
        flask.ext.socketio.emit('internal/stats/%s' % meta['widgetId'], json.dumps(stats))
    rwmgmtapi.ws.poller_finish(meta, 'internal stats', run)

@app.route('/internal/stats', methods=['GET'])
def internal_stats():
    stats = server_stats()
    return json.dumps(stats)


@app.route('/internal/stats', methods=['PUT'])
def internal_reset():
    wsgi_stats.reset(flask.request.environ)
    return OK

def server_stats():
    sockets = websocket.server.sockets
    worker_urls = {}
    nworkers = 0
    for socket_id, socket in sockets.items():
        if '/rwapp' in socket.active_ns:
            workers = socket.active_ns['/rwapp'].poller.workers
            nworkers += len(workers)
            for worker_id, worker in workers.items():
                count(worker_urls, worker['desc'])

    stats = {
        'websocket' : {
            'workers' : nworkers,
            'sockets' : len(sockets),
            'subscriptions' : worker_urls
        },
        'rest' : wsgi_stats_to_dict(wsgi_stats.all)
    }

    return stats

# WSGI by session stats

@websocket.on('internal/wsgi', namespace='/rwapp')
def ws_wsgi(meta):
    if not rwmgmtapi.ws.poller_init(meta):
        return

    stats = require_wsgi_stats(meta['statsId'])

    @flask.copy_current_request_context
    def run():
        data = wsgi_stats_to_dict(stats)
        flask.ext.socketio.emit('internal/wsgi/%s' % meta['widgetId'], json.dumps(data))

    rwmgmtapi.ws.poller_finish(meta, 'internal wsgi stats', run)

@app.route('/internal/wsgi/create', methods=['POST'])
def wsgi_create():
    stats = wsgi_stats.create_stats()
    return '{"statsId":"%s"}' % stats.stats_id

@app.route('/internal/wsgi/<stats_id>', methods=['GET'])
def wsgi_get(stats_id):
    stats = wsgi_stats_to_dict(require_wsgi_stats(stats_id))
    return json.dumps(stats)

@app.route('/internal/wsgi/<stats_id>/start', methods=['PUT'])
def wsgi_start(stats_id):
    require_wsgi_stats(stats_id).full = True
    return OK

@app.route('/internal/wsgi/<stats_id>/stop', methods=['PUT'])
def wsgi_stop(stats_id):
    require_wsgi_stats(stats_id).full = False
    return OK

@app.route('/internal/wsgi/<stats_id>/reset', methods=['PUT'])
def wsgi_reset(stats_id):
    require_wsgi_stats(stats_id).reset()
    return OK

@app.route('/internal/wsgi/<stats_id>/timings', methods=['GET'])
def wsgi_timings(stats_id):
    timings = require_wsgi_stats(stats_id).timings
    if not is_csv():
        return json.dumps(timings)
    return csv.writer(timings)

def require_wsgi_stats(stats_id):
    stats = wsgi_stats.find_stats_by_id(stats_id)
    if stats == None:
        flask.abort(404)
    return stats

@app.route('/internal/wsgi/<stats_id>/timeline', methods=['GET'])
def wsgi_timeline(stats_id):
    timeline = require_wsgi_stats(stats_id).timeline
    if not is_csv():
        return json.dumps(timeline)
    return csv.writer(timeline)

def is_csv():
    type = flask.request.accept_mimetypes.best_match(['application/json', 'text/csv'])
    return type == 'text/csv'


def wsgi_stats_to_dict(stats):
    data =  {
            'nRequests' : stats.n_requests,
            'totalTime' : stats.t_requests,
            'maxTime' : stats.max_request,
            'maxUrl' : stats.max_by_url
    }
    if stats.full:
        data['timings'] = stats.timings
        data['timeline'] = stats.timeline

    return data

def count(counts, key):
    if key in counts:
        counts[key] += 1
    else:
        counts[key] = 1
