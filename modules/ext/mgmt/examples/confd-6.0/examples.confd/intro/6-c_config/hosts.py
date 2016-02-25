"""
*********************************************************************
* ConfD Stats intro example                                         *
* Implements data provider and transactional callbacks              *
*                                                                   *
* (C) 2015 Tail-f Systems                                           *
* Permission to use this code as a starting point hereby granted    *
*                                                                   *
* See the README file for more information                          *
*********************************************************************
"""
from __future__ import print_function
import argparse
import os
import re
import select
import socket
import sys
import textwrap

import _confd
import _confd.dp as dp
import _confd.maapi as maapi

V = _confd.Value
hcp_calls_get_next = 0
hcp_calls_set_elem = 0
hcp_calls_create = 0
hcp_calls_remove = 0
hcp_calls_num_instances = 0
icp_calls_get_next = 0
icp_calls_set_elem = 0
icp_calls_create = 0
icp_calls_remove = 0
icp_calls_num_instances = 0


class Iface(object):
    def __init__(self, name, ip, mask, enabled):
        self.name = name
        self.ip = ip
        self.mask = mask
        self.enabled = enabled

    def __str__(self):
        basestr = "   iface: {0:>7} {1:>10} {2:>10} {3:>4}\n".format(
            self.name,
            self.ip,
            self.mask,
            self.enabled)
        return basestr

    def __repr__(self):
        str = "{0} {1} {2} {3}".format(
            self.name,
            self.ip,
            self.mask,
            self.enabled
        )
        return str


class Host(object):
    def __init__(self, name, domain, defgw, ifaces=[]):
        self.name = name
        self.domain = domain
        self.defgw = defgw
        self.ifaces = ifaces

    def add_iface(self, iface):
        self.ifaces.append(iface)

    def remove_iface(self, ifacename):
        for i in range(len(self.ifaces)):
            iface = self.ifaces[i]
            if iface.name == ifacename:
                self.ifaces.remove(iface)
                return
        print("No such interface <{0}>".format(ifacename))

    def show(self):
        print(str(self))

    def __str__(self):
        basestr = "Host {0:>10} {1:>10} {2:>10}\n".format(
            self.name,
            self.domain,
            self.defgw)
        for i in self.ifaces:
            basestr += str(i)

        return basestr

    def __repr__(self):
        basestr = "{0} {1} {2} {{".format(
            self.name,
            self.domain,
            self.defgw)
        for i in self.ifaces:
            basestr += "  {0}".format(repr(i))
        basestr += "  }\n"
        return basestr


class Dlist(object):
    def __init__(self):
        self.list = []
        self.locked = False

    # Help function which adds a new host, keeping the list ordered
    def add_host(self, host):
        # list should be sorted by name
        if len(self.list) == 0:
            self.list.append(host)
        else:
            for i in range(0, len(self.list)):
                if self.list[i].name < host.name:
                    self.list.insert(i, host)
                    break
                elif i == (len(self.list)-1):
                    self.list.append(host)

    def update_host(self, host):
        # list should be sorted by name
        for i in range(0, len(self.list)):
            if self.list[i].name == host.name:
                self.list.insert(i, host)
                return i
        # Did not find the host so return None
        return None

    def dump_db(self, filename):
        try:
            with open(filename, 'w') as f:
                dumpstr = ""
                for i in self.list:
                    dumpstr += repr(i)
                f.writelines(dumpstr)
        except IOError:
            print("Error writing file {0}\n".format(filename))
            return False
        # Otherwise all is ok
        return True

    def find_host(self, hostname):
        return next((x for x in self.list if x.name == hostname), None)

    def find_iface(self, hostname, ifname):
        host = self.find_host(hostname)
        if host:
            for i in host.ifaces:
                if i.name == ifname:
                    return i
        return None

    def restore(self, filename):
        self.list = []
        try:
            num_entries = 0
            with open(filename, 'r') as f:
                while True:
                    line = f.readline().rstrip()
                    if not line:
                        break
                    matching = re.match("(.*) (.*) (.*) \{ (.*) \}", line)
                    hostname = matching.group(1)
                    hostdomain = matching.group(2)
                    hostdefgw = matching.group(3)
                    ifaces = matching.group(4).split("  ")
                    ifacelist = []
                    # read ifaces until we find the ending curly bracket
                    # strip newlines
                    for iface in ifaces:
                        [ifname, ifipstr, ifmask, ifenabled] = iface.split()
                        ifacelist.append(
                            Iface(ifname, ifipstr, ifmask, ifenabled)
                        )
                    self.add_host(
                        Host(hostname, hostdomain, hostdefgw, ifacelist)
                    )
                    num_entries += 1
            print("Restoring {0} entries".format(num_entries))
        except IOError:
            print("Could not read entries from {0} \n".format(filename))
            return False
        # If no exception, return true
        return True

    def del_host(self, host):
        self.list.pop(host)

    def show(self):
        for i in self.list:
            print(i)

    def get_hostifacekey(self, keypath):
        keys = re.search(r'\{(.*)\}.*\{(.*)\}', str(keypath))
        (hostkey, ifacekey) = keys.groups()
        return [hostkey, ifacekey]

    def find_iface_tag(self, keypath):
        keys = re.search(r'\}/.*/(.*)', str(keypath))
        tag = keys.group(1)
        return tag

    def find_tag(self, keypath):
        keys = re.search(r'\}/(.*)', str(keypath))
        tag = keys.group(1)
        return tag

    def get_hostkey(self, keypath):
        m = re.search(r'\{(.*)\}', str(keypath))
        key = m.group(1)
        return key

    def default_db(self):
        # Initialize db to 2 hosts
        self.list = []
        buzz = Host(
            "buzz", "tail-f.com", "192.168.1.1",
            [Iface("eth0", "192.168.1.61", "255.255.255.0", "1"),
             Iface("eth1", "10.77.1.55", "255.255.255.0", "0"),
             Iface("lo", "127.0.0.1", "255.255.255.0", "1")]
        )
        earth = Host(
            "earth", "tail-f.com", "192.168.1.1",
            [Iface("bge0", "192.168.1.61", "255.255.255.0", "1"),
             Iface("lo0", "127.0.0.1", "255.0.0.0", "1")]
        )
        self.add_host(buzz)
        self.add_host(earth)


class TransCbs(object):
    ###########################################################################
    # transaction callbacks
    #
    # The installed init() function gets called everytime Confd
    # wants to establish a new transaction, Each NETCONF
    # command will be a transaction
    #
    # We can choose to create threads here or whatever, we
    # can choose to allocate this transaction to an already existing
    # thread. We must tell Confd which filedescriptor should be
    # Used for all future communication in this transaction
    # this has to be done through the call confd_trans_set_fd();

    def __init__(self, workersocket):
        self._workersocket = workersocket

    def cb_init(self, tctx):
        dp.trans_set_fd(tctx, self._workersocket)
        return _confd.CONFD_OK

        # This callback gets invoked at the end of the transaction
        # when ConfD has accumulated all write operations
        # we're guaranteed that
        # a) no more read ops will occur
        # b) no other transactions will run between here and tr_finish()
        #    for this transaction, i.e ConfD will serialize all transactions
        #  since we need to be prepared for abort(), we may not write
        # our data to the actual database, we can choose to either
        # copy the entire database here and write to the copy in the
        # following write operatons _or_ let the write operations
        # accumulate operations create(), set(), delete() instead of actually
        # writing

        # If our db supports transactions (which it doesn't in this
        # silly example, this is the place to do START TRANSACTION

    def cb_write_start(self, tctx):
        return _confd.CONFD_OK

    def cb_prepare(self, tctx):
        return _confd.CONFD_OK

    def cb_commit(self, tctx):
        print("commit called with {0}".format(tctx))

        return _confd.CONFD_OK

    def cb_finish(self, tctx):
        return _confd.CONFD_OK


class DbCbs(object):

    def __init__(self, datasocket, db):
        self.datasocket = datasocket
        self.db = db

    def cb_lock(self, tctx, dbname):
        self.db.locked = True
        return _confd.CONFD_OK

    def cb_unlock(self, tctx, dbname):
        self.db.locked = False
        return _confd.CONFD_OK

    def cb_delete_config(self, tctx, dbname):
        dp.db_seterr(tctx, "error from python")
        return _confd.CONFD_ERR

    def cb_add_checkpoint_running(self, tctx):
        if self.db.dump_db("RUNNING.ckp") is False:
            return _confd.CONFD_ERR
        return _confd.CONFD_OK

    def cb_del_checkpoint_running(self, tctx):
        os.unlink("RUNNING.ckp")
        return _confd.CONFD_OK

    def cb_activate_checkpoint_running(self, tctx):
        if self.db.restore("RUNNING.ckp"):
            return _confd.CONFD_OK
        else:
            return confd.CONFD_ERR


class HostDataCbs(object):
    def __init__(self, callpoint='hcp', db=None):
        self.callpoint = callpoint
        self.db = db

    def cb_get_elem(self, tctx, kp):
        hostkey = self.db.get_hostkey(kp)
        host = self.db.find_host(hostkey)
        if host is None:
            dp.data_reply_not_found(tctx)
            return _confd.CONFD_OK
        tag = self.db.find_tag(kp)
        val = None
        if tag == 'name':
            val = V(host.name)
        elif tag == 'domain':
            val = V(host.domain)
        elif tag == 'defgw':
            val = V(host.defgw, V.C_IPV4)
        else:
            return _confd.CONFD_ERR
        dp.data_reply_value(tctx, val)
        return _confd.CONFD_OK

    def cb_get_next(self, tctx, kp, next):
        global hcp_calls_get_next
        hcp_calls_get_next += 1
        if next == -1 and self.db.list == []:  # First call on empty list
            dp.data_reply_next_key(tctx, None, -1)
            return _confd.CONFD_OK
        elif next == -1:  # First call, nonempty list
            next = 0
        if next < len(self.db.list):
            curr = self.db.list[next]
            key = [V(curr.name)]  # The key of the host is its name
            dp.data_reply_next_key(tctx, key, next+1)
        else:  # last element
            dp.data_reply_next_key(tctx, None, 0)
        return _confd.CONFD_OK

    def cb_host_num_instances(self, tctx, kp):
        print("host_num_instances \n")
        global hcp_calls_num_instances
        hcp_calls_num_instances += 1
        host_num_instances = len(self.db.list)
        dp.data_reply(tctx, V(host_num_instances, _confd.C_INT))
        return _confd.CONFD_OK

    def cb_set_elem(self, txtc, kp, newval):
        print("host_set_elem \n")
        global hcp_calls_set_elem
        hcp_calls_set_elem += 1
        return _confd.CONFD_ACCUMULATE

    def cb_create(self, txtc, kp):
        print("host_create \n")
        global hcp_calls_create
        hcp_calls_create += 1
        return _confd.CONFD_ACCUMULATE

    def cb_remove(self, txtc, kp):
        print("host_remove \n")
        global hcp_calls_remove
        hcp_calls_remove += 1
        return _confd.CONFD_ACCUMULATE


class IfaceDataCbs(object):
    def __init__(self, callpoint='icp', db=None):
        self.callpoint = callpoint
        self.db = db

    # assuming the name of the host being configured is "earth"
    # the keypaths we get here will be like :
    # /hosts/host{earth}/interfaces/interface{eth0}/ip
    #   [6]  [5]   [4]     [3]        [2]     [1]   [0]
    # thus keypath->v[4][0] will refer to the name of the
    # host being configured
    # and  keypath->v[1][0] will refer to the name of the interface
    # being configured
    def cb_get_elem(self, tctx, kp):

        [hostkey, ifacekey] = self.db.get_hostifacekey(kp)
        iface = self.db.find_iface(hostkey, ifacekey)
        if iface is None:
            dp.data_reply_not_found(tctx)
            return _confd.CONFD_OK
        tag = self.db.find_iface_tag(kp)
        val = None
        if tag == 'name':
            val = V(iface.name)
        elif tag == 'ip':
            val = V(iface.ip, V.C_IPV4)
        elif tag == 'mask':
            val = V(iface.mask, V.C_IPV4)
        elif tag == 'enabled':
            if iface.enabled:
                val = V(True, V.C_BOOL)
            else:
                val = V(False, V.C_BOOL)
        else:
            return _confd.CONFD_ERR
        dp.data_reply_value(tctx, val)
        return _confd.CONFD_OK

    # keypath here will look like
    # /hosts/host{myhostname}/interfaces/interface
    def cb_get_next(self, tctx, kp, next):
        global icp_calls_get_next
        icp_calls_get_next += 1
        hostkey = self.db.get_hostkey(kp)
        host = self.db.find_host(hostkey)
        if next == -1:  # First call
            next = 0
        if host is None:
            dp.data_reply_next_key(tctx, None, -1)
            return _confd.CONFD_OK
        if host.ifaces == []:
            dp.data_reply_next_key(tctx, None, -1)
            return _confd.CONFD_OK
        if next < len(host.ifaces):
            iface = host.ifaces[next]
            key = [V(iface.name)]
            dp.data_reply_next_key(tctx, key, next+1)
        else:
            dp.data_reply_next_key(tctx, None, 0)
        return _confd.CONFD_OK

    def cb_host_num_instances(self, tctx, kp):
        return _confd.CONFD_OK

    def cb_set_elem(self, txtc, kp, newval):
        global icp_calls_set_elem
        icp_calls_set_elem += 1
        return _confd.CONFD_ACCUMULATE

    def cb_create(self, txtc, kp):
        global icp_calls_create
        icp_calls_create += 1
        return _confd.CONFD_ACCUMULATE

    def cb_remove(self, txtc, kp):
        global icp_calls_remove
        icp_calls_remove += 1
        return _confd.CONFD_ACCUMULATE


class Prompt(object):
    def __init__(self, db):
        self.db = db
        self.currhostname = None
        print("> "),
        sys.stdout.flush()

    def handle_stdin(self, line):
        if len(line) != 0:
            tok = line.split()
            if tok[0] == "show":
                if self.currhostname is None:
                    self.db.show()
                else:
                    host = self.db.find_host(self.currhostname)
                    if host is not None:
                        host.show()
            elif line == "host":
                print("usage: host <hname> | host <hname domain defgw>\n")
                self.currhostname = None
            elif line == "default":
                self.db.default_db()
            elif (tok[0] == 'host' and len(tok) > 1):
                db_host = self.db.find_host(tok[1])
                if db_host is not None:
                    self.currhostname = tok[1]
                else:  # Create a new host
                    try:
                        hostname = tok[1]
                        domain = tok[2]
                        defgw = tok[3]
                        self.db.add_host(Host(hostname, domain, defgw))
                    except IndexError:
                        print("usage: host <newhost> <domain> <defgw>\n")
                        self.currhostname = None
            elif (tok[0] == 'iface'):
                if self.currhostname is None:
                    print("Need to pick a host before we can create iface\n")
                else:
                    db_host = self.db.find_host(self.currhostname)
                    try:
                        name = tok[1]
                        ip = tok[2]
                        mask = tok[3]
                        enabled = tok[4]
                        db_host.add_iface(Iface(name, ip, mask, enabled))
                        self.db.update_host(db_host)
                    except IndexError:
                        print("usage: host <newhost> <domain> <defgw>\n")
                        self.currhostname = None
            elif (tok[0] == 'del'):
                if len(tok) == 1:
                    print("usage: del <hname | ifname>\n")
                elif self.currhostname is None:
                    # We are in the root-node, we should remove a host
                    db_host = self.db.find_host(tok[1])
                    if db_host is not None:
                        self.db.list.remove(db_host)
                else:
                    # We are in a 'host'-node, we should remove an iface
                    db_host = self.db.find_host(self.currhostname)
                    db_host.remove_iface(tok[1])
            elif (tok[0] == 'up'):
                self.currhostname = None
            elif (tok[0] == 'quit'):
                exit(0)
            elif (tok[0] == 'load'):
                if len(tok) != 2:
                    print("usage: load <file>\n")
                elif not self.db.restore(tok[1]):
                    print("failed to open {0} for reading \n".format(tok[1]))
            elif (tok[0] == 'dump'):
                fname = None
                if len(tok) < 2:
                    fname = "RUNNING.db"
                else:
                    fname = tok[1]
                if not self.db.dump_db(fname):
                    print("failed to dump to {0}\n".format(fname))
                else:
                    print("dumped to {0}".format(fname))
            else:
                print(
                    "show \n"
                    "host [hostname]\n"
                    "host <name> <domain> <defgw>    - to create new host\n"
                    "iface <name> <ip> <mask> <ena>  - to create new iface\n"
                    "del <hostname | ifacename>\n"
                    "up \n"
                    "quit \n"
                    "default      -  to load default db values\n"
                    "load <file>  -  to load db from <file> \n"
                    "dump <file>  -  to dump db to <file> \n"
                )

        if self.currhostname is None:
            print("> "),
            sys.stdout.flush()
        else:
            print("["+self.currhostname+"]" + "> "),
            sys.stdout.flush()


def run(debuglevel):

    # In C we use confd_init() which sets the debug-level, but for Python the
    # call to confd_init() is done when we do 'import confd'.
    # Therefore we need to set the debug level here:
    _confd.set_debug(debuglevel, sys.stderr)

    # init library
    ctx = dp.init_daemon("hosts_daemon")

    # initialize our simple database
    db = Dlist()
    if db.restore("RUNNING.ckp"):
        print("Restoring from checkpoint\n")
    elif db.restore("RUNNING.db"):
        print("Restoring from RUNNING.db\n")
    else:
        print("Starting with empy DB\n")

    db.show()

    maapisock = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
    ctlsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
    wrksock = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
    try:
        maapi.connect(maapisock, '127.0.0.1', _confd.CONFD_PORT)
        dp.connect(ctx,
                   ctlsock,
                   dp.CONTROL_SOCKET,
                   '127.0.0.1',
                   _confd.CONFD_PORT,
                   '/')
        dp.connect(ctx,
                   wrksock,
                   dp.WORKER_SOCKET,
                   '127.0.0.1',
                   _confd.CONFD_PORT,
                   '/')

        maapi.load_schemas(maapisock)

        # Transaction and db callbacks
        tcb = TransCbs(wrksock)
        dcb = DbCbs(wrksock, db)
        dp.register_trans_cb(ctx, tcb)
        dp.register_db_cb(ctx, dcb)

        # Read/Write callbacks
        host_cbks = HostDataCbs('hcp', db)
        iface_cbks = IfaceDataCbs('icp', db)
        if dp.register_data_cb(ctx, 'hcp', host_cbks) == _confd.CONFD_ERR:
            print("Failed to register host cb\n")
            exit(1)
        if dp.register_data_cb(ctx, 'icp', iface_cbks):
            print("Failed to register interface cb\n")
            exit(1)

        dp.register_done(ctx)

        p = Prompt(db)
        try:
            _r = [sys.stdin, ctlsock, wrksock]
            _w = []
            _e = []

            while (True):
                (r, w, e) = select.select(_r, _w, _e, 1)
                for rs in r:
                    if rs.fileno() == ctlsock.fileno():
                        try:
                            dp.fd_ready(ctx, ctlsock)
                        except (_confd.error.Error) as e:
                            if e.confd_errno is not _confd.ERR_EXTERNAL:
                                raise e
                    elif rs.fileno() == wrksock.fileno():
                        try:
                            dp.fd_ready(ctx, wrksock)
                        except (_confd.error.Error) as e:
                            if e.confd_errno is not _confd.ERR_EXTERNAL:
                                raise e
                    elif rs == sys.stdin:
                        p.handle_stdin(sys.stdin.readline().rstrip())

        except KeyboardInterrupt:
            print("\nCtrl-C pressed\n")

    finally:
        ctlsock.close()
        wrksock.close()
        dp.release_daemon(ctx)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="",
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument('-dl', '--debuglevel', choices=['q', 'd', 't', 'p'],
                        help=textwrap.dedent(
                            '''\
                            set the debug level:
                                q = quiet (i.e. no) debug
                                d = debug level debug
                                t = trace level debug
                                p = proto level debug
                            '''))
    args = parser.parse_args()
    print("Args = {0}".format(args))

    confd_debug_level = _confd.TRACE

    if args.debuglevel == 'q':
        confdDebugLevel = _confd.SILENT
    elif args.debuglevel == 'd':
        confdDebugLevel = _confd.DEBUG
    elif args.debuglevel == 't':
        confdDebugLevel = _confd.TRACE
    elif args.debuglevel == 'p':
        confdDebugLevel = _confd.PROTO

    run(confd_debug_level)
