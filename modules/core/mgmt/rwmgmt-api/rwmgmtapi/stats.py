
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import time

# Multiplexor to global stats and individual session stats
class SessionAwareStats(object):
    def __init__(self):
        self.stats_id_generator = 0
        self.sessions = {}
        self.all = self.create_stats()

    def record_start(self, environ, module_id):
        token = self.all.record_start(module_id)
        session_stats = self.find_stats_by_environ(environ)
        if session_stats:
            session_stats.record_start(module_id)
        return token

    def record_end(self, environ, token):
        self.all.record_end(token)
        session_stats = self.find_stats_by_environ(environ)
        if session_stats:
            session_stats.record_end(token)

    def reset(self, environ):
        self.all.reset()
        session_stats = self.find_stats_by_environ(environ)
        if session_stats:
            session_stats.reset()

    def create_stats(self):
        self.stats_id_generator += 1
        stats_id = 's' + str(self.stats_id_generator)
        stats = Stats(stats_id)
        self.sessions[stats_id] = stats
        return stats

    def find_stats_by_environ(self, environ):
        if 'HTTP_X_STATS' in environ:
            return self.find_stats_by_id(environ['HTTP_X_STATS'])
        return None

    def find_stats_by_id(self, session_id):
        if session_id in self.sessions:
            return self.sessions[session_id]
        return None

# General purpose stats collector that keeps track of timings.
class Stats(object):

    def __init__(self, stats_id):
        self.reset()
        self.stats_id = stats_id
        self.full = True

    def record_start(self, module_id):
        return {'t': time.time(),
         'm': module_id}

    def record_end(self, token):
        t1 = time.time()
        duration = t1 - token['t']
        token['d'] = duration
        self.t_requests += duration
        self.max_request = max(self.max_request, duration)
        max_value(self.max_by_url, token['m'], duration)
        self.n_requests += 1
        if self.full:
            self.timeline.append(token)
            timings = None
            if token['m'] in self.timings:
                timings = self.timings[token['m']]
            else:
                timings = []
                self.timings[token['m']] = timings
            timings.append(duration)

    def reset(self):
        self.t_requests = 0
        self.n_requests = 0
        self.max_request = 0
        self.max_by_url = {}
        self.timeline = []
        self.timings = {}


def max_value(bucket, id, n):
    if id in bucket:
        bucket[id] = max(bucket[id], n)
    else:
        bucket[id] = n
