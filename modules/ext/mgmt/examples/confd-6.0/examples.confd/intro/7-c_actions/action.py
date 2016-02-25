"""
*********************************************************************
* ConfD Actions intro example                                       *
* Implements a couple of actions                                    *
*                                                                   *
* (C) 2015 Tail-f Systems                                           *
* Permission to use this code as a starting point hereby granted    *
*                                                                   *
* See the README file for more information                          *
*********************************************************************
"""
from __future__ import print_function
import socket
import select

from config_ns import ns
import _confd
import _confd.dp as dp
import _confd.error as confd_errors


def make_tag_value(ns_hash, tag, value):
    """
    Wrapper to create a _confd.TagValue
    """
    return _confd.TagValue(_confd.XmlTag(ns_hash, tag),
                           _confd.Value(value))


class ActionCallbacks(object):

    """
    The action callbacks needed.
    """

    def __init__(self, worker_sock):
        self.wsock = worker_sock

    def cb_init(self, uinfo):
        print("init_action called")
        dp.action_set_fd(uinfo, self.wsock)

    def cb_action(self, uinfo, name, keypath, params):
        print("action called")
        for i, param in enumerate(params):
            print("param", i, "value", param.v)
        self.uinfo = uinfo
        self.name = name
        self.params = params
        fun = self.action_switch()
        action_result = fun()
        if action_result is not None:
            return action_result

    def cb_abort(self, uinfo):
        print("Aborting outstanding action")
        # We need to clean  up the worker socket by replying
        dp.action_delayed_reply_error(uinfo, "aborted")

    def action_switch(self):
        """
        Hacky python switch
        """
        return {
            ns.config_reboot: lambda: print("reboot"),
            ns.config_restart: self.restart,
            ns.config_reset: self.reset,
            ns.config_abort_test: lambda: _confd.DELAYED_RESPONSE
        }[self.name.tag]

    def restart(self):
        """
        tailf:action restart handling
        """
        print("restart")
        params = self.params
        uinfo = self.uinfo
        # params[0] is mode
        mode_value = str(params[0].v)

        # if we get mode_value == error1, we reply with generic error
        if mode_value == "error1":
            return _confd.CONFD_ERR

        # if we get mode_value == error2, we reply with specific error
        if mode_value == "error2":
            dp.action_seterr(uinfo, "myfail")
            return _confd.CONFD_ERR

        # otherwise, we create a result string with mode-result-...
        res = mode_value + "-result"
        k = 1
        n = len(params)
        while k < n:
            print("k:", k, "tag:", params[k].tag)
            if params[k].tag == ns.config_debug:
                res = res + "-debug"
            if params[k].tag == ns.config_foo:
                res = res + "-foo"
                if k + 1 < n:
                    if params[k + 1].tag == ns.config_debug:
                        res = res + "-debug"
                    k += 1
            k += 1

        result = [
            make_tag_value(ns.hash, ns.config_time, res)
        ]
        dp.action_reply_values(uinfo, result)

    def reset(self):
        """
        tailf:action reset handling
        """
        print("reset")
        when_value = self.params[0].v
        res = str(when_value) + "-result"
        result = [
            make_tag_value(ns.hash, ns.config_time, res)
        ]
        dp.action_reply_values(self.uinfo, result)


def connect(dctx, csock, wsock):
    """
    Connect the sockets
    """
    # Create the first control socket, all requests to
    # create new transactions arrive here
    dp.connect(dx=dctx,
               sock=csock,
               type=dp.CONTROL_SOCKET,
               ip='127.0.0.1',
               port=4565)

    # Also establish a workersocket, this is the most simple
    # case where we have just one ctlsock and one workersock
    dp.connect(dx=dctx,
               sock=wsock,
               type=dp.WORKER_SOCKET,
               ip='127.0.0.1',
               port=4565)


def read_data(dctx, sock):
    try:
        dp.fd_ready(dctx, sock)
    except (confd_errors.Error) as e:
        # Callback error
        if e.confd_errno is _confd.ERR_EXTERNAL:
            print(str(e))
        else:
            raise e


def poll_loop(dctx, ctrl_sock, worker_sock):
    """
    Check for I/O
    """
    _r = [ctrl_sock, worker_sock]
    _w = []
    _e = []

    try:
        while True:
            r, w, e = select.select(_r, _w, _e)

            for rs in r:
                if rs == ctrl_sock:
                    read_data(dctx=dctx, sock=ctrl_sock)
                elif rs == worker_sock:
                    read_data(dctx=dctx, sock=worker_sock)

    except KeyboardInterrupt:
        print("\nCtrl-C pressed\n")


def action_main():
    ctrl_sock = socket.socket()
    worker_sock = socket.socket()

    dctx = dp.init_daemon("actions_daemon")

    connect(dctx=dctx,
            csock=ctrl_sock,
            wsock=worker_sock)

    # register the action handler callback object
    acb = ActionCallbacks(worker_sock=worker_sock)
    dp.register_action_cbs(dctx, 'reboot-point', acb)

    dp.register_done(dctx)
    print("register_done called")

    try:
        poll_loop(dctx, ctrl_sock, worker_sock)
    finally:
        worker_sock.close()
        ctrl_sock.close()
        dp.release_daemon(dctx)

if __name__ == "__main__":
    action_main()
