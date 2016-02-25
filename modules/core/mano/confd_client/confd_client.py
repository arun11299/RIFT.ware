#!/usr/bin/env python2

# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import subprocess
import contextlib
import rift.auto.proxy
import sys
import os
import time
import rw_peas
import requests

from gi.repository import RwMcYang

# NOTE: This cript measures the single threaded performance
# This also gives an idea about latency
# To get throughput numbers may need multiple parallel clients


yang = rw_peas.PeasPlugin('yangmodel_plugin-c', 'YangModelPlugin-1.0')
yang_model_api = yang.get_interface('Model')
yang_model = yang_model_api.alloc()
mc_module = yang_model_api.load_module(yang_model, 'rw-mc')

@contextlib.contextmanager
def start_confd():
    print("Starting confd")
    proc = subprocess.Popen(["./usr/bin/rw_confd"])
    try:
        yield
    finally:
        print("Killing confd")
        proc.kill()

@contextlib.contextmanager
def start_confd_client():
    print("Starting confd_client")
    proc = subprocess.Popen(["{}/.build/modules/core/mc/src/core_mc-build/confd_client/confd_client".format(
        os.environ["RIFT_ROOT"])
        ])
    try:
        yield
    finally:
        proc.kill()
        print("Starting confd_client")

def run_rpc_perf_test(proxy, num_rpcs=1):
    start_time = time.time()

    for i in range(1, num_rpcs + 1):
        start = RwMcYang.StartLaunchpadInput()
        start.federation_name = "lp_%s" % i
        print(proxy.rpc(start.to_xml(yang_model)))

    stop_time = time.time()

    print("Retrieved %s rpc in %s seconds" % (num_rpcs, stop_time - start_time))
    return (stop_time - start_time)


def run_federation_config_http_perf_test(num_federations=1):
    session = requests.Session()

    start_time = time.time()
    for i in range(1, num_federations + 1):
        req = session.post(
                url="http://localhost:8008/api/config",
                json={"federation": {"name": "foo_%s" % i}},
                headers={'Content-Type': 'application/vnd.yang.data+json'},
                auth=('admin', 'admin')
                )
        req.raise_for_status()
    stop_time = time.time()

    print("Configured %s federations using restconf in %s seconds" % (num_federations, stop_time - start_time))
    return (stop_time - start_time)

def run_opdata_get_opdata_perf_test(proxy, num_gets=1):
    start_time = time.time()

    for i in range(1, num_gets + 1):
        print(proxy.get_from_xpath(filter_xpath="/opdata"))
        pass

    stop_time = time.time()
    print("Retrieved %s opdata in %s seconds" % (num_gets, stop_time - start_time))
    return (stop_time - start_time)

def run_federation_config_perf_test(proxy, num_federations=1):
    start_time = time.time()

    for i in range(1, num_federations + 1):
        fed = RwMcYang.FederationConfig()
        fed.name = "foobar_%s" % i
        proxy.merge_config(fed.to_xml(yang_model))

    stop_time = time.time()

    print("Configured %s federations using netconf in %s seconds" % (num_federations, stop_time - start_time))
    return (stop_time - start_time)

def run_federation_get_config_perf_test(proxy, num_gets=1):
    start_time = time.time()

    for i in range(1, num_gets + 1):
        proxy.get_config(filter_xpath="/federation")

    stop_time = time.time()

    print("Retrieved %s federations in %s seconds" % (num_gets, stop_time - start_time))
    return (stop_time - start_time)

def main():
    with start_confd():
        with start_confd_client():
            nc_proxy = rift.auto.proxy.NetconfProxy()
            nc_proxy.connect()
            n_fed = 10;
            n_fed_get = 100
            n_opdata_get = 100
            n_rpc = 100
            config_time = run_federation_config_perf_test(nc_proxy, num_federations=n_fed)
            config_get_time = run_federation_get_config_perf_test(nc_proxy, num_gets=n_fed_get)
            opdata_get_time = run_opdata_get_opdata_perf_test(nc_proxy, num_gets=n_opdata_get)
            rpc_time = run_rpc_perf_test(nc_proxy, num_rpcs=n_rpc)

            print("")
            print("..............................................")
            print("CONFD Performance Results Using Netconf Client")
            print("..............................................")
            print("Rate of config writes: %d" % (n_fed/config_time))
            print("Rate of config reads : %d" % (n_fed_get/config_get_time))
            print("Rate of opdata reads : %d" % (n_opdata_get/opdata_get_time))
            print("Rate of rpc calls    : %d" % (n_rpc/rpc_time))
            print("* Config read is reading a list with %d entries" % n_fed)
            print("* Opdata read is reading a list with 5 entries")
            print("..............................................")

if __name__ == "__main__":
    if "RIFT_ROOT" not in os.environ:
        print("Must be in rift shell to run.")
        sys.exit(1)

    os.chdir(os.environ["RIFT_INSTALL"])
    main()

