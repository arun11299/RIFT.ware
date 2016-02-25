#!/usr/bin/env python

# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#


import logging
import os
import pprint
import sys
import argparse

import rift.vcs.api
import rift.vcs.compiler
import rift.vcs.compiler.constraints
import rift.vcs.manifest
import rift.vcs.vms
import rift.vcs.core
from rift.vcs.ext import ClassProperty
from argparse import Action

gdb_enabled = False
rift_install = os.environ['RIFT_INSTALL']
rift_root = os.environ['RIFT_ROOT']
ut_src=os.environ['RIFT_ROOT'] + "/modules/core/mgmt/rwuagent/test/utframework"
op_dir=os.environ['RIFT_INSTALL'] + "/tmp/agent_ut"

class TestTasklet(rift.vcs.Tasklet):
    """
    This class represents a TestTasklet for uagent UT framework.
    """

    def __init__(self, name="RW.TestTasklet", uid=None):
        """Creates a TestTasklet object.

        Arguments:
            name - the name of the tasklet
            uid  - a unique identifier

        """
        super(TestTasklet, self).__init__(name=name, uid=uid)

    plugin_name = ClassProperty("testtasklet")
    plugin_directory = ClassProperty("./usr/lib/rift/plugins/testtasklet")

class MgmtVM(rift.vcs.VirtualMachine):
    """
    This class represents a management VM.
    """

    def __init__(self, name=None, confd_args="", *args, **kwargs):
        """Creates a MgmtVM object.

        Arguments:
            name          - the name of the tasklet

        """
        name = "RW_VM_MGMT" if name is None else name
        super(MgmtVM, self).__init__(name=name, *args, **kwargs)

        self.add_proc(rift.vcs.MsgBrokerTasklet())
        self.add_proc(rift.vcs.DtsRouterTasklet())
        self.add_tasklet(rift.vcs.uAgentTasklet())
        self.add_proc(rift.vcs.RestconfTasklet())
        self.add_proc(rift.vcs.procs.RiftCli())

        # Generate confd config file
        confd = rift.vcs.Confd()
        confd._exe = ut_src + "/ut_confd"
        confd._args = confd_args
        self.add_proc(confd)
        self.add_proc(TestTasklet())

def main(argv=sys.argv[1:]):
  
   parser = argparse.ArgumentParser()
   parser.add_argument('--with-gdb',
                       action='store_true',
                       help="Start the system in GDB")
   parser.add_argument('--reuse-confd-ws',
                       action='store_true',
                       help="Reuse confd workspace directory")
   parser.add_argument('--no-clean-exit',
                       action='store_true',
                       help="Do not clean workspace") 
   
   args = parser.parse_args(argv)

   # Create op directories
   if not os.path.exists(op_dir):
      os.makedirs(op_dir)
      
   VM='127.0.0.1'
   collapsed = True

   logging.basicConfig(format='%(asctime)-15s %(levelname)s %(message)s')
   logging.getLogger().setLevel(logging.DEBUG)

   colony = rift.vcs.core.Colony(name='test', uid=1)
   mgmt = rift.vcs.core.Cluster(name='mgmt')
   colony.add_cluster(mgmt)

   confd_args = ""
   if args.reuse_confd_ws:
       confd_args += "  --reuse-confd-ws"
   if args.no_clean_exit:
       confd_args += "  --no-clean-exit"
   mgmt.add_virtual_machine(MgmtVM(ip=VM, confd_args=confd_args))

   # Construct the system
   sysinfo = rift.vcs.core.SystemInfo(
            mode='ethsim',
            collapsed=collapsed,
            zookeeper=rift.vcs.manifest.RaZookeeper(zake=collapsed, master_ip=VM),
            colonies=[colony],
            multi_broker=True
    )

    # Compile the manifest
   compiler = rift.vcs.compiler.LegacyManifestCompiler()
   compiler.system_constraints = [cons for cons in compiler.system_constraints if type(cons) is not rift.vcs.compiler.constraints.AdjacentConfdRestconfTasklets ]

   _, manifest = compiler.compile(sysinfo)

   # Change the startup schema
   manifest.system_schema = "ut-composite"

   # Change the CLI manifest file
   racli = manifest.find_by_class(rift.vcs.manifest.RaCliProc)
   cli_manifest = ut_src + "/cli_ut.xml"
   racli.native_args = racli.native_args.replace("cli_rwfpath.xml", cli_manifest)

   manifest_file = op_dir + "/manifest.xml"

   vcs = rift.vcs.api.RaVcsApi()
   vcs.manifest_generate_xml(manifest, manifest_file)
   os.environ['RIFT_NO_SUDO_REAPER'] = '1'

   os.chdir(os.environ['RIFT_INSTALL'])
   vcs.exec_rwmain(manifest_file, args.with_gdb)

if __name__ == "__main__":
    main()
