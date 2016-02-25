#!/usr/bin/env python
"""
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

@file lp_test.py
@author Austin Cormier (Austin.Cormier@riftio.com)
@author Paul Laidler (Paul.Laidler@riftio.com)
@date 11/03/2015
@brief Launchpad System Test
"""

import json
import logging
import os
import pytest
import shlex
import requests
import subprocess
import time
import uuid
import rift.auto.session

from gi.repository import (
    RwMcYang,
    NsdYang,
    NsrYang,
    RwNsrYang,
    VnfrYang,
    VldYang,
    RwVnfdYang,
    RwLaunchpadYang,
    RwBaseYang
)

logging.basicConfig(level=logging.DEBUG)


@pytest.fixture(scope='module')
def mc_proxy(request, mgmt_session):
    return mgmt_session.proxy(RwMcYang)


@pytest.fixture(scope='module')
def launchpad_session(request, mc_proxy, mgmt_domain_name, session_type):
    launchpad_host = mc_proxy.get("/mgmt-domain/domain[name='%s']/launchpad/ip_address" % mgmt_domain_name)

    if session_type == 'netconf':
        launchpad_session = rift.auto.session.NetconfSession(host=launchpad_host)
    elif session_type == 'restconf':
        launchpad_session = rift.auto.session.RestconfSession(host=launchpad_host)

    launchpad_session.connect()
    rift.vcs.vcs.wait_until_system_started(launchpad_session, 600)
    return launchpad_session


@pytest.fixture(scope='module')
def launchpad_proxy(request, launchpad_session):
    return launchpad_session.proxy(RwLaunchpadYang)


@pytest.fixture(scope='module')
def vnfd_proxy(request, launchpad_session):
    return launchpad_session.proxy(RwVnfdYang)

@pytest.fixture(scope='module')
def vnfr_proxy(request, launchpad_session):
    return launchpad_session.proxy(VnfrYang)

@pytest.fixture(scope='module')
def vld_proxy(request, launchpad_session):
    return launchpad_session.proxy(VldYang)


@pytest.fixture(scope='module')
def nsd_proxy(request, launchpad_session):
    return launchpad_session.proxy(NsdYang)


@pytest.fixture(scope='module')
def nsr_proxy(request, launchpad_session):
    return launchpad_session.proxy(NsrYang)


@pytest.fixture(scope='module')
def rwnsr_proxy(request, launchpad_session):
    return launchpad_session.proxy(RwNsrYang)


@pytest.fixture(scope='module')
def base_proxy(request, launchpad_session):
    return launchpad_session.proxy(RwBaseYang)


def create_nsr(nsd_id, input_param_list, cloud_account_name):
    """
    Create the NSR record object

    Arguments:
         nsd_id             -  NSD id
         input_param_list - list of input-parameter objects

    Return:
         NSR object
    """
    nsr = RwNsrYang.YangData_Nsr_NsInstanceConfig_Nsr()

    nsr.id = str(uuid.uuid4())
    nsr.name = "nsr_name"
    nsr.short_name = "nsr_short_name"
    nsr.description = "This is a description"
    nsr.nsd_ref = nsd_id
    nsr.admin_status = "ENABLED"
    nsr.input_parameter.extend(input_param_list)
    nsr.cloud_account = cloud_account_name

    return nsr


def upload_descriptor(logger, descriptor_file, host="127.0.0.1"):
    curl_cmd = 'curl -F "descriptor=@{file}" http://{host}:4567/api/upload'.format(
            file=descriptor_file,
            host=host,
            )
    logger.debug("Uploading descriptor %s using cmd: %s", descriptor_file, curl_cmd)
    stdout = subprocess.check_output(shlex.split(curl_cmd), universal_newlines=True)

    json_out = json.loads(stdout)
    transaction_id = json_out["transaction_id"]

    return transaction_id


class DescriptorOnboardError(Exception):
    pass


def wait_onboard_transaction_finished(logger, transaction_id, timeout=600, host="127.0.0.1"):
    logger.info("Waiting for onboard trans_id %s to complete", transaction_id)
    uri = 'http://%s:4567/api/upload/%s/state' % (host, transaction_id)
    elapsed = 0
    start = time.time()
    while elapsed < timeout:
        reply = requests.get(uri)
        state = reply.json()
        if state["status"] == "success":
            break

        if state["status"] != "pending":
            raise DescriptorOnboardError(state)

        time.sleep(1)
        elapsed = time.time() - start

    if state["status"] != "success":
        raise DescriptorOnboardError(state)

    logger.info("Descriptor onboard was successful")



@pytest.mark.setup('pingpong')
@pytest.mark.depends('launchpad')
@pytest.mark.usefixtures('splat_launchpad')
@pytest.mark.incremental
class TestPingPongStart(object):
    def test_onboard_ping_vnfd(self, logger, mc_proxy, mgmt_domain_name, vnfd_proxy, ping_vnfd_package_file):
        launchpad_host = mc_proxy.get("/mgmt-domain/domain[name='%s']/launchpad/ip_address" % mgmt_domain_name)
        logger.info("Onboarding ping_vnfd package: %s", ping_vnfd_package_file)
        trans_id = upload_descriptor(logger, ping_vnfd_package_file, launchpad_host)
        wait_onboard_transaction_finished(logger, trans_id, host=launchpad_host)

        catalog = vnfd_proxy.get_config('/vnfd-catalog')
        vnfds = catalog.vnfd
        assert len(vnfds) == 1, "There should only be a single vnfd"
        vnfd = vnfds[0]
        assert vnfd.name == "ping_vnfd"

    def test_onboard_pong_vnfd(self, logger, mc_proxy, mgmt_domain_name, vnfd_proxy, pong_vnfd_package_file):
        launchpad_host = mc_proxy.get("/mgmt-domain/domain[name='%s']/launchpad/ip_address" % mgmt_domain_name)
        logger.info("Onboarding pong_vnfd package: %s", pong_vnfd_package_file)
        trans_id = upload_descriptor(logger, pong_vnfd_package_file, launchpad_host)
        wait_onboard_transaction_finished(logger, trans_id, host=launchpad_host)

        catalog = vnfd_proxy.get_config('/vnfd-catalog')
        vnfds = catalog.vnfd
        assert len(vnfds) == 2, "There should be two vnfds"
        assert "pong_vnfd" in [vnfds[0].name, vnfds[1].name]

    def test_onboard_ping_pong_nsd(self, logger, mc_proxy, mgmt_domain_name, nsd_proxy, ping_pong_nsd_package_file):
        launchpad_host = mc_proxy.get("/mgmt-domain/domain[name='%s']/launchpad/ip_address" % mgmt_domain_name)
        logger.info("Onboarding ping_pong_nsd package: %s", ping_pong_nsd_package_file)
        trans_id = upload_descriptor(logger, ping_pong_nsd_package_file, launchpad_host)
        wait_onboard_transaction_finished(logger, trans_id, host=launchpad_host)

        catalog = nsd_proxy.get_config('/nsd-catalog')
        nsds = catalog.nsd
        assert len(nsds) == 1, "There should only be a single nsd"
        nsd = nsds[0]
        assert nsd.name == "ping_pong_nsd"

    def test_instantiate_ping_pong_nsr(self, logger, nsd_proxy, nsr_proxy, rwnsr_proxy, base_proxy, cloud_account_name):

        def verify_input_parameters (running_config, config_param):
            """
            Verify the configured parameter set against the running configuration
            """
            for run_input_param in running_config.input_parameter:
                if (input_param.xpath == config_param.xpath and
                    input_param.value == config_param.value):
                    return True

            assert False, ("Verification of configured input parameters: { xpath:%s, value:%s} "
                          "is unsuccessful.\nRunning configuration: %s" % (config_param.xpath,
                                                                           config_param.value,
                                                                           running_nsr_config.input_parameter))

        catalog = nsd_proxy.get_config('/nsd-catalog')
        nsd = catalog.nsd[0]

        input_parameters = []
        descr_xpath = "/nsd:nsd-catalog/nsd:nsd[nsd:id='%s']/nsd:description" % nsd.id
        descr_value = "New NSD Description"
        in_param_id = str(uuid.uuid4())

        input_param_1= NsrYang.YangData_Nsr_NsInstanceConfig_Nsr_InputParameter(
                                                                xpath=descr_xpath,
                                                                value=descr_value)

        input_parameters.append(input_param_1)

        nsr = create_nsr(nsd.id, input_parameters, cloud_account_name)

        logger.info("Instantiating the Network Service")
        rwnsr_proxy.create_config('/ns-instance-config/nsr', nsr)

        nsr_opdata = rwnsr_proxy.get('/ns-instance-opdata')
        nsrs = nsr_opdata.nsr

        # Verify the input parameter configuration
        running_config = rwnsr_proxy.get_config("/ns-instance-config/nsr[id='%s']" % nsr.id)
        for input_param in input_parameters:
            verify_input_parameters(running_config, input_param)

        assert len(nsrs) == 1
        assert nsrs[0].ns_instance_config_ref == nsr.id

        xpath = "/ns-instance-opdata/nsr[ns-instance-config-ref='{}']/operational-status".format(nsr.id)
        rwnsr_proxy.wait_for(xpath, "running", timeout=240)


@pytest.mark.teardown('pingpong')
@pytest.mark.depends('launchpad')
@pytest.mark.usefixtures('splat_launchpad')
@pytest.mark.incremental
class TestPingPongTeardown(object):
    def test_terminate_nsr(self, nsr_proxy, vnfr_proxy, rwnsr_proxy, logger):
        """
        Terminate the instance and check if the record is deleted.

        Asserts:
        1. NSR record is deleted from instance-config.

        """
        logger.debug("Terminating Ping Pong NSR")

        nsr_path = "/ns-instance-config"
        nsr = rwnsr_proxy.get_config(nsr_path)

        ping_pong = nsr.nsr[0]
        rwnsr_proxy.delete_config("/ns-instance-config/nsr[id='{}']".format(ping_pong.id))

        nsr = rwnsr_proxy.get_config(nsr_path)
        assert nsr is None

        # Termination tests
        vnfr = "/vnfr-catalog/vnfr"
        vnfrs = vnfr_proxy.get(vnfr, list_obj=True)
        assert vnfrs is None or len(vnfrs.vnfr) == 0

        # nsr = "/ns-instance-opdata/nsr"
        # nsrs = rwnsr_proxy.get(nsr, list_obj=True)
        # assert len(nsrs.nsr) == 0

    def test_delete_records(self, nsd_proxy, vnfd_proxy):
        """Delete the NSD & VNFD records

        Asserts:
            The records are deleted.
        """
        nsds = nsd_proxy.get("/nsd-catalog/nsd", list_obj=True)
        for nsd in nsds.nsd:
            xpath = "/nsd-catalog/nsd[id='{}']".format(nsd.id)
            nsd_proxy.delete_config(xpath)

        nsds = nsd_proxy.get("/nsd-catalog/nsd", list_obj=True)
        assert nsds is None or len(nsds.nsd) == 0

        vnfds = vnfd_proxy.get("/vnfd-catalog/vnfd", list_obj=True)
        for vnfd_record in vnfds.vnfd:
            xpath = "/vnfd-catalog/vnfd[id='{}']".format(vnfd_record.id)
            vnfd_proxy.delete_config(xpath)

        vnfds = vnfd_proxy.get("/vnfd-catalog/vnfd", list_obj=True)
        assert vnfds is None or len(vnfds.vnfd) == 0
