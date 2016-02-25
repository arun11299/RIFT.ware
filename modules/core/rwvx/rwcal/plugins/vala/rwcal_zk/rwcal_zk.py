
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import logging

import kazoo.exceptions
from gi.repository import (
    GObject,
    RwCal,
    RwTypes)

import rw_status
import rwlogger

import rift.cal
import rift.cal.rwzk

logger = logging.getLogger('rwcal')

rwstatus = rw_status.rwstatus_from_exc_map({
                IndexError: RwTypes.RwStatus.NOTFOUND,
                KeyError: RwTypes.RwStatus.NOTFOUND,

                kazoo.exceptions.NodeExistsError: RwTypes.RwStatus.EXISTS,
                kazoo.exceptions.NoNodeError: RwTypes.RwStatus.NOTFOUND,
                kazoo.exceptions.NotEmptyError: RwTypes.RwStatus.NOTEMPTY,
                kazoo.exceptions.LockTimeout: RwTypes.RwStatus.TIMEOUT,

                rift.cal.rwzk.UnlockedError: RwTypes.RwStatus.NOTCONNECTED,
           })

class ZookeeperPlugin(GObject.Object, RwCal.Zookeeper):
    def __init__(self):
      GObject.Object.__init__(self)
      self._cli = None

    @rwstatus
    def do_create_server_config(self, zkid, unique_ports, server_names):
        rift.cal.rwzk.create_server_config(zkid, unique_ports, server_names)

    @rwstatus
    def do_server_start(self, zkid):
        rift.cal.rwzk.server_start(zkid)

    @rwstatus
    def do_kazoo_init(self, unique_ports, server_names):
        if self._cli is not None:
            if isinstance(rift.cal.rwzk.Kazoo, self._cli):
                return
            else:
                raise AttributeError('Zookeeper client was already initialized')

        self._cli = rift.cal.rwzk.Kazoo()
        self._cli.client_init(unique_ports, server_names)

    @rwstatus
    def do_zake_init(self):
        if self._cli is not None:
            if isinstance(rift.cal.rwzk.Zake, self._cli):
                return
            else:
                raise AttributeError('Zookeeper client was already initialized')

        self._cli = rift.cal.rwzk.Zake()
        self._cli.client_init('', '')

    @rwstatus
    def do_lock(self, path, timeout):
        if timeout == 0.0:
            timeout = None

        self._cli.lock_node(path, timeout)

    @rwstatus
    def do_unlock(self, path):
        self._cli.unlock_node(path)

    def do_locked(self, path):
        try:
            return self._cli.locked(path)
        except kazoo.exceptions.NoNodeError:
            # A node that doesn't exist can't really be locked.
            return False

    @rwstatus
    def do_create(self, path, closure=None):
        if not closure:
            self._cli.create_node(path)
        else:
            self._cli.acreate_node(path, closure.callback)

    def do_exists(self, path):
        return self._cli.exists(path)

    @rwstatus(ret_on_failure=[""])
    def do_get(self, path, closure=None):
        if not closure:
            data = self._cli.get_node_data(path)
            return data.decode()
        self._cli.aget_node_data(path, closure.store_data, closure.callback)
        return 0


    @rwstatus
    def do_set(self, path, data, closure=None):
        if not closure:
            self._cli.set_node_data(path, data.encode(), None)
        else:
            self._cli.aset_node_data(path, data.encode(), closure.callback)


    @rwstatus(ret_on_failure=[[]])
    def do_children(self, path, closure=None):
        if not closure:
            return self._cli.get_node_children(path)
        self._cli.aget_node_children(path, closure.store_data, closure.callback)
        return 0

    @rwstatus
    def do_rm(self, path, closure=None):
        if not closure:
            self._cli.delete_node(path)
        else:
            self._cli.adelete_node(path, closure.callback)

    @rwstatus
    def do_register_watcher(self, path, closure):
        self._cli.register_watcher(path, closure.callback)

    @rwstatus
    def do_unregister_watcher(self, path, closure):
        self._cli.unregister_watcher(path, closure.callback)




def main():
    @rwstatus
    def blah():
        raise IndexError()

    a = blah()
    assert(a == RwTypes.RwStatus.NOTFOUND)

    @rwstatus({IndexError: RwTypes.RwStatus.NOTCONNECTED})
    def blah2():
        """Some function"""
        raise IndexError()

    a = blah2()
    assert(a == RwTypes.RwStatus.NOTCONNECTED)
    assert(blah2.__doc__ == "Some function")

if __name__ == '__main__':
    main()

