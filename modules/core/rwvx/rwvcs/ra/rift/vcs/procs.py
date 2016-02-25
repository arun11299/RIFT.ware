
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import os
import logging
import tempfile

from . import core

logger = logging.getLogger(__name__)

class Webserver(core.NativeProcess):
    """
    This class represents a webserver process.
    """

    def __init__(self,
            uid=None,
            name="RW.Webserver",
            ui_dir="./usr/share/rwmgmt-ui",
            confd_host="localhost",
            confd_port="8008",
            uagent_port=None,
            config_ready=True,
            ):
        """Creates a Webserver object.

        Arguments:
            uid         - a unique identifier
            name        - the name of the process
            ui_dir      - the path to the UI resources
            confd_host  - the host that confd is on
            confd_port  - the port that confd is communicating on
            uagent_port - the port that the uAgent is communicating on
            config_ready - config readiness check enable

        """
        self.ui_dir = ui_dir
        self.confd_host = confd_host
        self.confd_port = confd_port
        self.uagent_port = uagent_port

        super(Webserver, self).__init__(
                uid=uid,
                name=name,
                exe="./usr/local/bin/rwmgmt-api-standalone",
                config_ready=config_ready,
                )

    @property
    def args(self):
        return ' '.join([
            '--ui_dir {}'.format(self.ui_dir),
            '--server localhost:{}'.format(self.uagent_port),
            '--log-level CRITICAL',
            '--confd admin:admin@{}:{}'.format(self.confd_host, self.confd_port),
            ])

class RestconfNative(core.NativeProcess):
    """
    This class represents a Rift Restconf Server process.
    """

    def __init__(self,
            uid=None,
            name="RW.Restconf",
            confd_host="127.0.0.1",
            confd_port="2022",
            rest_port="8888",
            schema_name="rw-composite",
            config_ready=True,
            ):
        """Creates a Rift Restconf Server object.

        Arguments:
            uid         - a unique identifier
            name        - the name of the process
            confd_host  - the host that confd is on
            confd_port  - the port that confd is communicating on
            rest_port   - the port that the restconf server listens on
            config_ready - config readiness check enable
        """

        super(Restconf, self).__init__(
                uid=uid,
                name=name,
                exe="./usr/local/bin/rwrestconf_standalone",
                config_ready=config_ready,
                )

        self.confd_host = confd_host
        self.confd_port = confd_port
        self.rest_port = rest_port
        self.schema_name = schema_name

    @property
    def args(self):
        return ' '.join([
            '--netconf-ip {}'.format(self.confd_host),
            '--netconf-port {}'.format(self.confd_port),
            '--restconf-port {}'.format(self.rest_port),
            '--schema {}'.format(self.schema_name),
            ])

class RedisCluster(core.NativeProcess):
    """
    This class represents a redis cluster process.
    """

    def __init__(self,
            uid=None,
            name="RW.RedisCluster",
            num_nodes=3,
            init_port=3152,
            config_ready=True,
            ):
        """Creates a RedisCluster object.

        Arguments:
            name      - the name of the process
            uid       - a unique identifier
            num_nodes - the number of nodes in the cluster
            init_port - the nodes in the cluster are assigned sequential ports
                        starting at the value given by 'init_port'.
            config_ready - config readiness check enable

        """
        args = './usr/bin/redis_cluster.py -c -n {} -p {}'
        super(RedisCluster, self).__init__(
                uid=uid,
                name=name,
                exe='python',
                args=args.format(num_nodes, init_port),
                config_ready=config_ready,
                )


class RedisServer(core.NativeProcess):
    """
    This class represents a redis server process.
    """

    def __init__(self, uid=None, name="RW.RedisServer", port=None, config_ready=True):
        """Creates a RedisServer object.

        Arguments:
            name - the name of the process
            uid  - a unique identifier
            port - the port to use for the server
            config_ready - config readiness check enable

        """
        # If the port is not specified, use the default for redis (NB: the
        # redis_cluster.py wrapper requires the init port to be specified so
        # something has to be provided).
        if port is None:
            port = '6379'

        super(RedisServer, self).__init__(
                uid=uid,
                name=name,
                exe='python',
                args= './usr/bin/redis_cluster.py -c -n 1 -p {}'.format(port),
                config_ready=config_ready,
                )


class UIServerLauncher(core.NativeProcess):
    """
    This class represents a UI Server Launcher.
    """

    def __init__(self, uid=None, name="RW.MC.UI", config_ready=True):
        """Creates a UI Server Launcher

        Arguments:
            uid  - a unique identifier
            name - the name of the process
            config_ready - config readiness check enable

        """
        super(UIServerLauncher, self).__init__(
                uid=uid,
                name=name,
                exe="./usr/share/rw.ui/webapp/scripts/launch_ui.sh",
                config_ready=config_ready,
                )
    @property
    def args(self):
        return ' '

class Confd(core.NativeProcess):
    """
    This class represents a confd process.
    """

    def __init__(self, uid=None, name="RW.Confd", config_ready=True):
        """Creates a Confd object.

        Arguments:
            uid  - a unique identifier
            name - the name of the process
            config_ready - config readiness check enable

        RIFT-8268 deprecates the direct use
        of Confd object.
        Use manifest.py to provide unique
        workspace to confd via uAgent command args
        """

        import subprocess
        hostname = subprocess.check_output(["hostnamectl", "--static"]).decode("ascii").strip("\n")
        super(Confd, self).__init__(
                uid=uid,
                name=name,
                exe="./usr/bin/rw_confd",
                args="--unique confd_persist_{}".format(hostname),
                config_ready=config_ready,
                )

class RiftCli(core.NativeProcess):
    """
    This class represents a Rift CLI process.
    """

    def __init__(self,
              uid=None,
              name="RW.CLI",
              schema_listing="cli_rwfpath_schema_listing.txt",
              netconf_host="127.0.0.1",
              netconf_port="2022",
              config_ready=True,
              ):
        """Creates a RiftCli object.

        Arguments:
            manifest_file - the file listing exported yang modules
            uid  - a unique identifier
            name - the name of the process
            netconf_host - IP/Host name where the Netconf server is listening
            netconf_port - Port on which Netconf server is listening
            config_ready - config readiness check enable

        """
        super(RiftCli, self).__init__(
                uid=uid,
                name=name,
                exe="./usr/bin/rwcli",
                interactive=True,
                config_ready=config_ready,
                )
        self.netconf_host = netconf_host
        self.netconf_port = netconf_port
        self.schema_listing = schema_listing
    @property
    def args(self):
        return ' '.join([
            '--netconf_host {}'.format(self.netconf_host),
            '--netconf_port {}'.format(self.netconf_port),
            '--schema_listing {}'.format(self.schema_listing),
            ])

class CrossbarServer(core.NativeProcess):
    """
    This class represents a Crossbar process used for DTS mock.
    """

    def __init__(self, uid=None, name="RW.Crossbar", config_ready=True):
        """Creates a CrossbarServer object.

        Arguments:
            uid  - a unique identifier
            name - the name of the process
            config_ready - config readiness check enable

        """
        super(CrossbarServer, self).__init__(
                uid=uid,
                name=name,
                exe="/usr/bin/crossbar",
                config_ready=config_ready,
                )

    @property
    def args(self):
        return ' '.join([
            "start", "--cbdir", "etc/crossbar/config", "--loglevel", "debug", "--logtofile",
            ])

