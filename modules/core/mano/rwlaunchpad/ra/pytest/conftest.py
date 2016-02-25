
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import pytest
import os

class PackageError(Exception):
    pass

@pytest.fixture(scope='session', autouse=True)
def cloud_account_name(request):
    '''fixture which returns the name used to identify the cloud account'''
    return 'cloud-0'

@pytest.fixture(scope='session')
def ping_pong_install_dir():
    install_dir = os.path.join(
        os.environ["RIFT_INSTALL"],
        "usr/rift/mano/examples/ping_pong_ns"
        )
    return install_dir

@pytest.fixture(scope='session')
def ping_vnfd_package_file(ping_pong_install_dir):
    ping_pkg_file = os.path.join(
            ping_pong_install_dir,
            "ping_vnfd_with_image.tar.gz",
            )
    if not os.path.exists(ping_pkg_file):
        raise_package_error()

    return ping_pkg_file


@pytest.fixture(scope='session')
def pong_vnfd_package_file(ping_pong_install_dir):
    pong_pkg_file = os.path.join(
            ping_pong_install_dir,
            "pong_vnfd_with_image.tar.gz",
            )
    if not os.path.exists(pong_pkg_file):
        raise_package_error()

    return pong_pkg_file


@pytest.fixture(scope='session')
def ping_pong_nsd_package_file(ping_pong_install_dir):
    ping_pong_pkg_file = os.path.join(
            ping_pong_install_dir,
            "ping_pong_nsd.tar.gz",
            )
    if not os.path.exists(ping_pong_pkg_file):
        raise_package_error()

    return ping_pong_pkg_file

