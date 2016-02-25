
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import abc
import asyncio
import functools
import logging
import os
import socket
import sys
import types
import unittest


import gi.repository.CF as cf
import gi.repository.RwMain as rwmain
import gi.repository.RwManifestYang as rwmanifest


import rift.tasklets


if sys.version_info < (3, 4, 4):
    asyncio.ensure_future = asyncio.async


class AbstractDTSTest(unittest.TestCase):
    """Provides the base components for setting up DTS related unit tests.

    The class provides 3 hooks for subclasses:
    1. configure_suite(Optional): Similar to setUpClass, configs
            related to entire suite goes here
    2. configure_test(Optional): Similar to setUp
    3. configure_schema(Mandatory): Schema of the yang module.
    4. configure_timeout(Optional): timeout for each test case, defaults to 5


    Note:  Each tests uses a list of asyncio.Events for staging through the
    test.  These are required here because we are bring up each coroutine
    ("tasklet") at the same time and are not implementing any re-try
    mechanisms.  For instance, this is used in numerous tests to make sure that
    a publisher is up and ready before the subscriber sends queries.  Such
    event lists should not be used in production software.
    """
    rwmain = None
    tinfo = None
    schema = None
    id_cnt = 0
    default_timeout = 0
    top_dir = __file__[:__file__.find('/modules/core/')]
    log_level = logging.WARN

    @classmethod
    def setUpClass(cls):
        """
        1. create a rwmain
        2. Add DTS Router and Broker tasklets. Sets a random port for the broker
        3. Triggers the configure_suite and configure_schema hooks.
        """
        sock = socket.socket()
        # Get an available port from OS and pass it on to broker.
        sock.bind(('', 0))
        port = sock.getsockname()[1]
        sock.close()
        rwmsg_broker_port = port
        os.environ['RWMSG_BROKER_PORT'] = str(rwmsg_broker_port)

        build_dir = os.path.join(cls.top_dir, '.build/modules/core/rwvx/src/core_rwvx-build')

        if 'MESSAGE_BROKER_DIR' not in os.environ:
            os.environ['MESSAGE_BROKER_DIR'] = os.path.join(build_dir, 'rwmsg/plugins/rwmsgbroker-c')

        if 'ROUTER_DIR' not in os.environ:
            os.environ['ROUTER_DIR'] = os.path.join(build_dir, 'rwdts/plugins/rwdtsrouter-c')

        msgbroker_dir = os.environ.get('MESSAGE_BROKER_DIR')
        router_dir = os.environ.get('ROUTER_DIR')

        manifest = rwmanifest.Manifest()
        manifest.init_phase.settings.rwdtsrouter.single_dtsrouter.enable = True

        cls.rwmain = rwmain.Gi.new(manifest)
        cls.tinfo = cls.rwmain.get_tasklet_info()

        # Run router in mainq. Eliminates some ill-diagnosed bootstrap races.
        os.environ['RWDTS_ROUTER_MAINQ'] = '1'
        cls.rwmain.add_tasklet(msgbroker_dir, 'rwmsgbroker-c')
        cls.rwmain.add_tasklet(router_dir, 'rwdtsrouter-c')

        cls.log = rift.tasklets.logger_from_tasklet_info(cls.tinfo)
        cls.log.setLevel(logging.DEBUG)

        fmt = logging.Formatter(
                '%(asctime)-23s %(levelname)-5s  (%(name)s@%(process)d:%(filename)s:%(lineno)d) - %(message)s')
        stderr_handler = logging.StreamHandler(stream=sys.stderr)
        stderr_handler.setFormatter(fmt)
        stderr_handler.setLevel(cls.log_level)
        logging.getLogger().addHandler(stderr_handler)

        cls.schema = cls.configure_schema()
        cls.default_timeout = cls.configure_timeout()
        cls.configure_suite(cls.rwmain)

        os.environ["PYTHONASYNCIODEBUG"] = "1"

    @abc.abstractclassmethod
    def configure_schema(cls):
        """
        Returns:
            yang schema.
        """
        raise NotImplementedError("configure_schema needs to be implemented")

    @classmethod
    def configure_timeout(cls):
        """
        Returns:
            Time limit for each test case, in seconds.
        """
        return 5

    @classmethod
    def configure_suite(cls, rwmain):
        """
        Args:
            rwmain (RwMain): Newly create rwmain instace, can be used to add
                    additional tasklets.
        """
        pass

    def setUp(self):
        """
        1. Creates an asyncio loop
        2. Triggers the hook configure_test
        """
        def scheduler_tick(self, *args):
            self.call_soon(self.stop)
            self.run_forever()

        # Init params: loop & timers
        self.loop = asyncio.new_event_loop()
        self.loop.set_debug(True)
        asyncio_logger = logging.getLogger("asyncio")
        asyncio_logger.setLevel(logging.DEBUG)

        self.loop.scheduler_tick = types.MethodType(scheduler_tick, self.loop)

        self.asyncio_timer = None
        self.stop_timer = None
        self.__class__.id_cnt += 1

        self.configure_test(self.loop, self.__class__.id_cnt)

    def configure_test(self, loop, test_id):
        """
        Args:
            loop (asyncio.BaseEventLoop): Newly created asyncio event loop.
            test_id (int): Id for tests.
        """
        pass

    def run_until(self, test_done, timeout=None):
        """
        Attach the current asyncio event loop to rwsched and then run the
        scheduler until the test_done function returns True or timeout seconds
        pass.

        Args:
            test_done (function): function which should return True once the test is
                    complete and the scheduler no longer needs to run.
            timeout (int, optional): maximum number of seconds to run the test.
        """
        timeout = timeout or self.__class__.default_timeout
        tinfo = self.__class__.tinfo

        def shutdown(*args):
            if args:
                self.log.debug('Shutting down loop due to timeout')

            if self.asyncio_timer is not None:
                tinfo.rwsched_tasklet.CFRunLoopTimerRelease(self.asyncio_timer)
                self.asyncio_timer = None

            if self.stop_timer is not None:
                tinfo.rwsched_tasklet.CFRunLoopTimerRelease(self.stop_timer)
                self.stop_timer = None

            tinfo.rwsched_instance.CFRunLoopStop()

        def tick(*args):
            self.loop.call_later(0.1, self.loop.stop)

            try:
                self.loop.run_forever()
            except KeyboardInterrupt:
                self.log.debug("Shutting down loop dur to keyboard interrupt")
                shutdown()

            if test_done():
                shutdown()

        self.asyncio_timer = tinfo.rwsched_tasklet.CFRunLoopTimer(
            cf.CFAbsoluteTimeGetCurrent(),
            0.1,
            tick,
            None)

        self.stop_timer = tinfo.rwsched_tasklet.CFRunLoopTimer(
            cf.CFAbsoluteTimeGetCurrent() + timeout,
            0,
            shutdown,
            None)

        tinfo.rwsched_tasklet.CFRunLoopAddTimer(
            tinfo.rwsched_tasklet.CFRunLoopGetCurrent(),
            self.stop_timer,
            tinfo.rwsched_instance.CFRunLoopGetMainMode())

        tinfo.rwsched_tasklet.CFRunLoopAddTimer(
            tinfo.rwsched_tasklet.CFRunLoopGetCurrent(),
            self.asyncio_timer,
            tinfo.rwsched_instance.CFRunLoopGetMainMode())

        tinfo.rwsched_instance.CFRunLoopRun()

        self.assertTrue(test_done())

    def new_tinfo(self, name):
        """
        Create a new tasklet info instance with a unique instance_id per test.
        It is up to each test to use unique names if more that one tasklet info
        instance is needed.

        @param name - name of the "tasklet"
        @return     - new tasklet info instance
        """
        # Accessing using class for consistency.
        ret = self.__class__.rwmain.new_tasklet_info(
                name,
                self.__class__.id_cnt)

        log = rift.tasklets.logger_from_tasklet_info(ret)
        log.setLevel(logging.DEBUG)

        stderr_handler = logging.StreamHandler(stream=sys.stderr)
        fmt = logging.Formatter(
                '%(asctime)-23s %(levelname)-5s  (%(name)s@%(process)d:%(filename)s:%(lineno)d) - %(message)s')
        stderr_handler.setFormatter(fmt)
        log.addHandler(stderr_handler)

        return ret


def async_test(f):
    """
    Runs the testcase within a coroutine using the current test cases loop.
    """
    @functools.wraps(f)
    def wrapper(*args, **kwargs):
        self = args[0]
        if not hasattr(self, "loop"):
            raise ValueError("Could not find loop attribute in first param")

        coro = asyncio.coroutine(f)
        future = coro(*args, **kwargs)
        task = asyncio.ensure_future(future, loop=self.loop)

        self.run_until(task.done)
        if task.exception() is not None:
            self.log.error("Caught exception during test: %s", str(task.exception()))
            raise task.exception()

    return wrapper
