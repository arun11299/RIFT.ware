#!/usr/bin/env python

# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#


import logging
import os
import pprint
import sys

import rift.vcs.api
import rift.vcs.compiler
import rift.vcs.manifest
import rift.vcs.vms

gdb_enabled = False
class CliVM(rift.vcs.VirtualMachine):
    """
    This class represents a CLI VM.
    """

    def __init__(self, name=None, *args, **kwargs):
        """Creates a CliVM object.

        Arguments:
            name - the name of the tasklet

        """
        name = "RW_VM_CLI" if name is None else name
        super(CliVM, self).__init__(name=name, *args, **kwargs)

        self.add_tasklet(rift.vcs.uAgentTasklet())
        self.add_proc(rift.vcs.procs.RiftCli());
        self.add_proc(rift.vcs.DtsPerfTasklet())

class MgmtVM(rift.vcs.VirtualMachine):
    """
    This class represents a management VM.
    """

    def __init__(self, name=None, *args, **kwargs):
        """Creates a MgmtVM object.

        Arguments:
            name          - the name of the tasklet

        """
        name = "RW_VM_MGMT" if name is None else name
        super(MgmtVM, self).__init__(name=name, *args, **kwargs)

        self.add_proc(rift.vcs.MsgBrokerTasklet())
        self.add_proc(rift.vcs.DtsRouterTasklet())
        self.add_proc(rift.vcs.DtsPerfTasklet())

        #Confd would need RestConf present
        #self.add_proc(rift.vcs.Confd())
        #self.add_proc(rift.vcs.RestconfTasklet())

        #self.add_proc(rift.vcs.Webserver())
        #self.add_proc(rift.vcs.RedisCluster())

class GenVM(rift.vcs.VirtualMachine):
    """
    This class represents a generic VM.
    """

    def __init__(self, name=None, *args, **kwargs):
        """Creates a GenVM object.

        Arguments:
            name          - the name of the tasklet

        """
        name = "RW_VM_GEN" if name is None else name
        super(GenVM, self).__init__(name=name, *args, **kwargs)

        self.add_proc(rift.vcs.DtsPerfTasklet())


def main():
    MASTER="10.0.23.169"
    VM1="10.0.23.175"
    VM2="10.0.23.143"
    collapsed = False

    if '-l' in sys.argv:
        MASTER='127.0.0.1'
        VM1='127.0.0.1'
        VM2='127.0.0.1'
        VM3='127.0.0.1'
        collapsed = True

    if '-g' in sys.argv:
        gdb_enabled = True

    logging.basicConfig(format='%(asctime)-15s %(levelname)s %(message)s')
    logging.getLogger().setLevel(logging.DEBUG)

    colony = rift.vcs.core.Colony(name='top', uid=1)

    #gen = rift.vcs.core.Cluster(name='gen')
    #colony.add_cluster(gen)
    #vm3 = GenVM(ip=VM2)
    #vm3.leader = True
    #gen.add_virtual_machine(vm3)


    mgmt = rift.vcs.core.Cluster(name='mgmt')
    colony.add_cluster(mgmt)

    vm2 = MgmtVM(ip=VM1)
    vm2.leader = True
    mgmt.add_virtual_machine(vm2)

    lead = CliVM(ip=MASTER)
    lead.leader = True
    colony.append(lead)

    multid=False
    if '-m' in sys.argv:
      multid=True

    # Construct the system
    sysinfo = rift.vcs.core.SystemInfo(
            mode='ethsim',
            collapsed=collapsed,
            zookeeper=rift.vcs.manifest.RaZookeeper(zake=collapsed, master_ip=MASTER),
            colonies=[colony],
            multi_broker=True,
            multi_dtsrouter=multid,

    )

    # Compile the manifest
    compiler = rift.vcs.compiler.LegacyManifestCompiler()
    _, manifest = compiler.compile(sysinfo)

    pwd = os.getcwd()
    os.chdir(os.environ['RIFT_INSTALL'])

    manifest_file = os.path.join(pwd, "manifest.xml")

    vcs = rift.vcs.api.RaVcsApi()
    vcs.manifest_generate_xml(manifest, manifest_file)


if __name__ == "__main__":
    main()

