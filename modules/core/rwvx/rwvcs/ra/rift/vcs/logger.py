"""
#
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
#
# @file: logger.py
# @author: Varun Prasad
# @date: 09/23/2015
# @brief: Contains classes to setup various log sinks.
# @detail: Each Logger class here has two functions.
#          1. To provide a data model to be used in config files
#          2. To set up and configure the sink based on the data provided.
"""

import abc
import imp
import logging
import multiprocessing
import os
import subprocess
import tempfile

import gi.repository.RwlogMgmtYang as rwlogmgmt
import gi.repository.RwLogYang as rwlog
import gi.repository.RwYang
import rift.auto.session
import rift.vcs.vcs
import rift.vcs.mgmt

import six


logger = logging.getLogger(__name__)


@six.add_metaclass(abc.ABCMeta)
class SinkInterface(object):
    """
    The SinkInterface provides couple of hooks that needs to be implemented
    1. pre_startup: Called before the system has started.
    2. post_startup: Called after the system has started.
    """

    @abc.abstractmethod
    def post_startup(self, proxy):
        pass

    @abc.abstractmethod
    def pre_startup(self):
        pass


class SyslogSink(SinkInterface):
    """
    Convenience class that provide all the initialization steps for setting up
    Syslog sink.

    """
    def __init__(
            self,
            name,
            ip="127.0.0.1",
            port=514,
            filters=None,
            install_loganalyzer=True):
        """
        Args:
            name (str): Syslog sink name
            ip (str, optional): Host IP to start the sink.
            port (int, optional): Port on which the Syslog listens.
            filters (list, optional): A list of dicts of the forms
                    {"<CATEGORY-NAME>": "<LOG_LEVEL>"}
            install_loganalyzer (bool, optional): Does an installation of
                    log analyzer
        """
        self.ip = ip
        self.port = port
        self.filters = {} if filters is None else filters
        self.name = name
        self.syslog_url = "http://{}/loganalyzer".format(self.ip)
        self.install_loganalyzer = install_loganalyzer

    def post_startup(self, proxy):
        """
        Converts the config data to Yang model and applies the config.
        """
        parsed_filters = []
        for filter_name, severity in self.filters.items():
            parsed_filters.append({
                    "name": filter_name,
                    "severity": severity})

        category = {"category": parsed_filters}
        config = {
                "syslog_viewer": self.syslog_url,
                "sink": [{"name": self.name,
                          "server_address": self.ip,
                          "port": self.port,
                          "filter": category}]
                }

        model = gi.repository.RwYang.Model.create_libncx()
        model.load_schema_ypbc(rwlogmgmt.get_schema())
        model.load_schema_ypbc(rwlog.get_schema())

        sink = rwlogmgmt.Logging.from_dict(config)
        xml = sink.to_xml_v2(model)
        proxy.merge_config(xml=xml)

    def pre_startup(self):
        """
        Does the following.

        1. Based on the config, renders the config file.
        2. Moves the config file to the specified host.
        3. Refreshes the rsyslog process
        """
        rift_install = os.environ['RIFT_INSTALL']
        rift_artifacts = os.environ['RIFT_ARTIFACTS']
        TEMPLATE_PATH = os.path.join(rift_install, "etc/syslog_template.conf")
        DEST_PATH = "/etc/rsyslog.d/rift.conf"
        ssh = "/usr/rift/bin/ssh_root"
        try:
            with open(TEMPLATE_PATH) as template_file:
                config_data = template_file.read().format(port=self.port)

            temp_file = tempfile.NamedTemporaryFile(
                    delete=False,
                    dir=rift_artifacts)

            temp_file.write(config_data.encode('UTF-8'))
            temp_file.close()

            logger.debug('Copying the SyslogSink temp from {} -> {}'.format(
                    TEMPLATE_PATH,
                    DEST_PATH))

            cmd = "cp {} {}".format(temp_file.name, DEST_PATH)
            subprocess.call([ssh, self.ip, cmd])

            rift_shell_cmd = "service rsyslog restart"

            logger.debug('Refreshing rsyslog instance')
            subprocess.call([ssh, self.ip, rift_shell_cmd])

            if self.install_loganalyzer:
                rift_root = os.environ['RIFT_INSTALL']
                rift_shell_cmd = os.path.join(
                        rift_root,
                        "etc/install_loganalyzer")
                rift_cmd_etc = "env RIFT_INSTALL='{}' '{}'".format(rift_root, rift_shell_cmd)

                logger.debug('Installing log analyzer')
                subprocess.call([ssh, self.ip, rift_cmd_etc])

        except IOError as e:
            logger.error("Unable to create config file %s, Will proceed with "
                         "the start by trying to use the existing file.",
                         str(e))


def worker(config_file, confd_ip):
    """
    Worker unit that will drive the SinkInterface methods. Atleast one of the
    two arguments needs to be provided
    """

    sink = None
    if config_file is not None:
        # loader for demo info
        with open(config_file) as cfg_file:
            module = imp.load_source('', config_file, cfg_file)

        if getattr(module, "sink_config", None) is not None:
            sink = module.sink_config

        # Try to figure out the IP from the config file, if a confd_ip is
        # not provided.
        if confd_ip is None:
            confd_vm = module.sysinfo.find_ancestor_by_class(rift.vcs.Confd)
            confd_ip = confd_vm.ip

    if sink is None:
        # If no sink config has been provided then fallback to default.
        sink = SyslogSink(name="syslog", ip=confd_ip)
    sink.pre_startup()

    mgmt_session = rift.auto.session.NetconfSession(host=confd_ip)
    mgmt_session.connect()
    rift.vcs.vcs.wait_until_system_started(mgmt_session)

    # For now retaining the old proxy method. Will be eventually moved to
    # session module's proxy.
    proxy = rift.auto.proxy.NetconfProxy(confd_ip)
    proxy.connect()

    sink.post_startup(proxy)


def configure_sink(config_file, confd_ip):
    """
    A convenience method that simplifies the setup for the external callers.
    """
    process = multiprocessing.Process(
            target=worker,
            args=(config_file, confd_ip))
    process.start()
