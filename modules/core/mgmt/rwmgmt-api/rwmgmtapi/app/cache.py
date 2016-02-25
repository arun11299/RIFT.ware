
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import flask.ext.cache
import functools
import logging
from flask import current_app, request

logger = logging.getLogger(__name__)

# Flask-Cache doesn't take response format into consideration (ACCEPT) and therefore
# XML and JSON formats can get mised up.
#   https://github.com/thadeusb/flask-cache/issues/107
class Cache(flask.ext.cache.Cache):
    def __init__(self, app=None, with_jinja2_ext=True, config=None):
        flask.ext.cache.Cache.__init__(self, app=app, with_jinja2_ext=with_jinja2_ext, config=config)

    def cached(self, timeout=None, key_prefix='rest/%s', unless=None):
        def decorator(f):
            @functools.wraps(f)
            def decorated_function(*args, **kwargs):
                #: Bypass the cache entirely.
                if callable(unless) and unless() is True:
                    return f(*args, **kwargs)

                try:
                    cache_key = decorated_function.make_cache_key(*args, **kwargs)
                    rv = self.cache.get(cache_key)
                except Exception:
                    if current_app.debug:
                        raise
                    logger.exception("Exception possibly due to cache backend.")
                    return f(*args, **kwargs)

                if rv is None:
                    rv = f(*args, **kwargs)
                    try:
                        self.cache.set(cache_key, rv,
                                   timeout=decorated_function.cache_timeout)
                    except Exception:
                        if current_app.debug:
                            raise
                        logger.exception("Exception possibly due to cache backend.")
                        return f(*args, **kwargs)
                return rv

            def make_cache_key(*args, **kwargs):
                if callable(key_prefix):
                    return key_prefix()
                enc = request.headers['Accept'] + '~'
                if '%s' in key_prefix:
                    return enc + (key_prefix % request.path)
                return enc + key_prefix

            decorated_function.uncached = f
            decorated_function.cache_timeout = timeout
            decorated_function.make_cache_key = make_cache_key

            return decorated_function

        return decorator
