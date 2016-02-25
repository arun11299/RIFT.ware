
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import pytest
import subprocess
import sys

import rift.auto.log_scraper
import rift.auto.session
import rift.vcs.vcs
import logging

from gi.repository import RwMcYang

@pytest.fixture(scope='session', autouse=True)
def cloud_account_name(request):
    '''fixture which returns the name used to identify the cloud account'''
    return 'cloud-0'

@pytest.fixture(scope='session', autouse=True)
def mgmt_domain_name(request):
    '''fixture which returns the name used to identify the mgmt_domain'''
    return 'mgmt-0'

@pytest.fixture(scope='session', autouse=True)
def vm_pool_name(request):
    '''fixture which returns the name used to identify the vm resource pool'''
    return 'vm-0'

@pytest.fixture(scope='session', autouse=True)
def network_pool_name(request):
    '''fixture which returns the name used to identify the network resource pool'''
    return 'net-0'

@pytest.fixture(scope='session', autouse=True)
def port_pool_name(request):
    '''fixture which returns the name used to identify the port resource pool'''
    return 'port-0'

@pytest.fixture(scope='session', autouse=True)
def sdn_account_name(request):
    '''fixture which returns the name used to identify the sdn account'''
    return 'sdn-0'

@pytest.fixture(scope='session', autouse=True)
def sdn_account_type(request):
    '''fixture which returns the account type used by the sdn account'''
    return 'odl'

@pytest.fixture(scope='session', autouse=True)
def cloud_account(request, cloud_account_name, cloud_host, cloud_type):
    '''fixture which returns an instance of RwMcYang.CloudAccount

    Arguments:
        request            - pytest fixture request
        cloud_account_name - fixture: name used for cloud account
        cloud_host         - fixture: cloud host address
        cloud_type         - fixture: cloud account type

    Returns:
        An instance of RwMcYang.CloudAccount
    '''
    account = None

    if cloud_type == 'lxc':
        account = RwMcYang.CloudAccount(
                name=cloud_account_name,
                account_type='cloudsim')

    elif cloud_type == 'openstack':
        username='pluto'
        password='mypasswd'
        auth_url='http://{cloud_host}:5000/v3/'.format(cloud_host=cloud_host)
        project_name='demo'
        mgmt_network='private'
        account = RwMcYang.CloudAccount.from_dict({
                'name':cloud_account_name,
                'account_type':'openstack',
                'openstack':{
                  'admin':True,
                  'key':username,
                  'secret':password,
                  'auth_url':auth_url,
                  'tenant':project_name,
                  'mgmt_network':mgmt_network}})

    return account

@pytest.fixture(scope='session')
def scraper_proxy(mgmt_session):
    '''fixture which returns a proxy to RwMcYang

    Arguments:
        mgmt_session  - mgmt_session fixture - instance of a rift.auto.session class
    '''
    return mgmt_session.proxy(RwMcYang)

@pytest.fixture(scope='session')
def _launchpad_scraper_session(scraper_proxy, mgmt_domain_name):
    '''fixture which returns an instance of rift.auto.log_scraper.RemoteLogScraper to scrape the launchpad

    Arguments:
        scraper_proxy - proxy used by scraper to determine launchpad host
        mgmt_domain_name - the management domain created for the launchpad
    '''
    return rift.auto.log_scraper.RemoteLogScraper('/var/log/launchpad_console.log')

@pytest.fixture(scope='function')
def splat_launchpad(request, logger, scraper_proxy, _launchpad_scraper_session, mgmt_domain_name):
    '''
    # splat_ : the function returned by this fixture will be run when a
    #          test that uses this fixture fails

    This fixture when used will automatically scrape the launchpad log at
    the beginning of each test.

    When an exception occurs the portion of the log generated during the test
    will be printed to stderr to be captured.

    Arguments:
        request - fixture request object
        _launchpad_scraper_session - instance of rift.auto.log_scraper.RemoteLogScraper targeting the launchpad console log
    '''

    if not _launchpad_scraper_session.connected():
        def _cmp_response_exists(expected, response):
            if response is not None and response != "":
                return True
            return False

        try:
            host = scraper_proxy.wait_for(
                    "/mgmt-domain/domain[name='%s']/launchpad/ip_address" % mgmt_domain_name,
                    'exists',
                    timeout=200,
                    compare=_cmp_response_exists)
            _launchpad_scraper_session.connect(host)
        except Exception as e:
            logger.warn('Failed to determine launchpad ip address: %s', str(e))

    def on_test_failure():
        '''When a test fails scrape and emit new entries in the launchpad log
        '''
        scraped = _launchpad_scraper_session.scrape()
        if scraped:
            print("=== START LAUNCHPAD LOG ===")
            print(scraped)
            print("=== END LAUNCHPAD LOG ===")

    def fin_scrape():
        '''At the end of each test scrape and discard entries from the launchpad log
        '''
        _launchpad_scraper_session.scrape()
    request.addfinalizer(fin_scrape)

    return on_test_failure

@pytest.fixture(scope='session')
def launchpad_scraper(_launchpad_scraper_session):
    '''Fixture exposing the scraper used to scrape the launchpad console log

    Arguments:
        _launchpad_scraper_session - instance of rift.auto.log_scraper.RemoteLogScraper targeting the launchpad console log
    '''
    return _launchpad_scraper_session

