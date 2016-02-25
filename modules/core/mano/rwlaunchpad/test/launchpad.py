#!/usr/bin/env python3

# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#


import logging
import os
import resource
import socket
import sys
import subprocess
import shlex
import netifaces

import rift.vcs
import rift.vcs.demo
import rift.vcs.vms

import rift.rwcal.cloudsim
import rift.rwcal.cloudsim.net

from rift.vcs.ext import ClassProperty

logger = logging.getLogger(__name__)


class NsmTasklet(rift.vcs.core.Tasklet):
    """
    This class represents a network services manager tasklet.
    """

    def __init__(self, name='network-services-manager', uid=None):
        """
        Creates a NsmTasklet object.

        Arguments:
            name  - the name of the tasklet
            uid   - a unique identifier
        """
        super(NsmTasklet, self).__init__(name=name, uid=uid)

    plugin_directory = ClassProperty('./usr/lib/rift/plugins/rwnsmtasklet')
    plugin_name = ClassProperty('rwnsmtasklet')


class VnsTasklet(rift.vcs.core.Tasklet):
    """
    This class represents a network services manager tasklet.
    """

    def __init__(self, name='virtual-network-service', uid=None):
        """
        Creates a VnsTasklet object.

        Arguments:
            name  - the name of the tasklet
            uid   - a unique identifier
        """
        super(VnsTasklet, self).__init__(name=name, uid=uid)

    plugin_directory = ClassProperty('./usr/lib/rift/plugins/rwvnstasklet')
    plugin_name = ClassProperty('rwvnstasklet')


class VnfmTasklet(rift.vcs.core.Tasklet):
    """
    This class represents a virtual network function manager tasklet.
    """

    def __init__(self, name='virtual-network-function-manager', uid=None):
        """
        Creates a VnfmTasklet object.

        Arguments:
            name  - the name of the tasklet
            uid   - a unique identifier
        """
        super(VnfmTasklet, self).__init__(name=name, uid=uid)

    plugin_directory = ClassProperty('./usr/lib/rift/plugins/rwvnfmtasklet')
    plugin_name = ClassProperty('rwvnfmtasklet')


class ResMgrTasklet(rift.vcs.core.Tasklet):
    """
    This class represents a Resource Manager tasklet.
    """

    def __init__(self, name='Resource-Manager', uid=None):
        """
        Creates a ResMgrTasklet object.

        Arguments:
            name  - the name of the tasklet
            uid   - a unique identifier
        """
        super(ResMgrTasklet, self).__init__(name=name, uid=uid)

    plugin_directory = ClassProperty('./usr/lib/rift/plugins/rwresmgrtasklet')
    plugin_name = ClassProperty('rwresmgrtasklet')


class MonitorTasklet(rift.vcs.core.Tasklet):
    """
    This class represents a tasklet that is used to monitor NFVI metrics.
    """

    def __init__(self, name='nfvi-metrics-monitor', uid=None):
        """
        Creates a MonitorTasklet object.

        Arguments:
            name  - the name of the tasklet
            uid   - a unique identifier

        """
        super(MonitorTasklet, self).__init__(name=name, uid=uid)

    plugin_directory = ClassProperty('./usr/lib/rift/plugins/rwmonitor')
    plugin_name = ClassProperty('rwmonitor')


class UIServer(rift.vcs.NativeProcess):
    def __init__(self, name="RW.MC.UI"):
        super(UIServer, self).__init__(
                name=name,
                exe="./usr/share/rw.ui/webapp/scripts/launch_ui.sh",
                )

    @property
    def args(self):
        return ' '


class ComposerUI(rift.vcs.NativeProcess):
    def __init__(self, name="RW.COMPOSER.UI"):
        super(ComposerUI, self).__init__(
                name=name,
                exe="./usr/share/composer/scripts/launch_composer.sh",
                )

    @property
    def args(self):
        return ' '


class ConfigManagerTasklet(rift.vcs.core.Tasklet):
    """
    This class represents a Resource Manager tasklet.
    """

    def __init__(self, name='Configuration-Manager', uid=None):
        """
        Creates a ConfigManagerTasklet object.

        Arguments:
            name  - the name of the tasklet
            uid   - a unique identifier
        """
        super(ConfigManagerTasklet, self).__init__(name=name, uid=uid)

    plugin_directory = ClassProperty('./usr/lib/rift/plugins/rwconmantasklet')
    plugin_name = ClassProperty('rwconmantasklet')


class Demo(rift.vcs.demo.Demo):
    def __init__(self, with_cntr_mgr=False):

        procs = [
            rift.vcs.RiftCli(),
            rift.vcs.DtsRouterTasklet(),
            rift.vcs.MsgBrokerTasklet(),
            rift.vcs.RestconfTasklet(),
            rift.vcs.RestPortForwardTasklet(),
            rift.vcs.CalProxy(),
            NsmTasklet(),
            VnfmTasklet(),
            VnsTasklet(),
            MonitorTasklet(),
            UIServer(),
            ComposerUI(),
            ConfigManagerTasklet(),
            rift.vcs.Launchpad(),
            ResMgrTasklet(),
            ]

        if with_cntr_mgr:
            procs.append(rift.vcs.ContainerManager())

        super(Demo, self).__init__(
            # Construct the system. This system consists of 1 cluster in 1
            # colony. The master cluster houses CLI and management VMs
            sysinfo = rift.vcs.SystemInfo(
                    colonies=[
                            rift.vcs.Colony(
                                name='master',
                                clusters=[
                                    rift.vcs.VirtualMachine(
                                        name='vm-launchpad',
                                        ip='127.0.0.1',
                                        tasklets=[
                                            rift.vcs.uAgentTasklet(),
                                            ],
                                        procs=procs,
                                        ),
                                    ]
                                )
                            ]
                        ),

            # Define the generic portmap.
            port_map = {},

            # Define a mapping from the placeholder logical names to the real
            # port names for each of the different modes supported by this demo.
            port_names = {
                'ethsim': {
                },
                'pci': {
                }
            },

            # Define the connectivity between logical port names.
            port_groups = {},
        )


def clear_salt_keys():
    # clear all the previously installed salt keys
    logger.info("Removing all unconnected salt keys")
    stdout = subprocess.check_output(
            shlex.split('salt-run manage.down'),
            universal_newlines=True,
            )

    down_minions = stdout.splitlines()

    for line in down_minions:
        salt_id = line.strip().replace("- ", "")
        logger.info("Removing old unconnected salt id: %s", salt_id)
        minion_keys_stdout = subprocess.check_output(
                shlex.split('salt-key -f {}'.format(salt_id)),
                universal_newlines=True)

        minion_keys = minion_keys_stdout.splitlines()
        for key_line in minion_keys:
            if "Keys" in key_line:
                continue

            key_split = key_line.split(":")
            if len(key_split) < 2:
                continue

            key = key_split[0]

            # Delete the minion key
            logger.info("Deleting minion %s key: %s", salt_id, key)
            subprocess.check_call(shlex.split('salt-key -d {} -y'.format(key)))


def main(argv=sys.argv[1:]):
    logging.basicConfig(format='%(asctime)-15s %(levelname)s %(message)s')

    # Create a parser which includes all generic demo arguments
    parser = rift.vcs.demo.DemoArgParser()

    parser.add_argument(
            '--with-cntr-mgr',
            action='store_true',
            help='Enable the container manager tasklet'
            )

    args = parser.parse_args(argv)

    # Disable loading any kernel modules for the launchpad VM
    # since it doesn't need it and it will fail within containers
    os.environ["NO_KERNEL_MODS"] = "1"

    if args.with_cntr_mgr:
        # In order to reliably module test, the virbr0 bridge
        # with IP 192.168.122.1 must exist before we start executing.
        # This is because in expanded mode, we need to use a container
        # accessible IP address for zookeeper clients.
        rift.rwcal.cloudsim.net.virsh_initialize_default()
        clear_salt_keys()

    #load demo info and create Demo object
    demo = Demo(args.with_cntr_mgr)

    # Create the prepared system from the demo
    system = rift.vcs.demo.prepared_system_from_demo_and_args(demo, args)

    confd_ip = socket.gethostbyname(socket.gethostname())
    intf = netifaces.ifaddresses('eth0')
    if intf and netifaces.AF_INET in intf and len(intf[netifaces.AF_INET]):
       confd_ip = intf[netifaces.AF_INET][0]['addr']
    rift.vcs.logger.configure_sink(config_file=None, confd_ip=confd_ip)

    # Start the prepared system
    system.start()


if __name__ == "__main__":
    resource.setrlimit(resource.RLIMIT_CORE, (resource.RLIM_INFINITY, resource.RLIM_INFINITY) )
    try:
        main()
    except rift.vcs.demo.ReservationError:
        print("ERROR: unable to retrieve a list of IP addresses from the reservation system")
        sys.exit(1)
    except rift.vcs.demo.MissingModeError:
        print("ERROR: you need to provide a mode to run the script")
        sys.exit(1)
    finally:
        os.system("stty sane")
