
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import _pytest.mark
import _pytest.runner

import itertools
import pytest
import re
import subprocess

from collections import OrderedDict

import rift.auto.log_scraper
import rift.auto.session
import rift.vcs.vcs
import logging

from gi.repository import RwMcYang

def pytest_addoption(parser):
    """pytest hook
    Add arguments options to the py.test command line

    Arguments:
        parser - An instance of _pytest.config.Parser
    """

    # --confd-host <addr>
    #   Host address of the confd instance
    parser.addoption("--confd-host", action="store", default="127.0.0.1")

    # --cloud-host <addr>
    #   Host address of cloud provider
    parser.addoption("--cloud-host", action="store", default="127.0.0.1")

    # --repeat <times>
    #   Number of times to repeat the entire test
    parser.addoption("--repeat", action="store", default=1)

    # --repeat-keyword <keywordexpr>
    #   Only repeat tests selected by pytest keyword expression
    parser.addoption("--repeat-keyword", action="store", default="")

    # --repeat-mark <markexpr>
    #   Only repeat tests selected by a pytest mark expression
    parser.addoption("--repeat-mark", action="store", default="")

    # The following options specify which session type to use
    # if multiple of these options are specified tests will run for each type specified

    # --netconf
    #   Provide a netconf session to test cases
    parser.addoption("--netconf", dest='session_type', action="append_const", const='netconf')

    # --restconf
    #   Provide a restconf session to test cases
    parser.addoption("--restconf", dest='session_type', action="append_const", const='restconf')

    # --lxc
    #   Use lxc cloud account
    parser.addoption("--lxc", dest="cloud_type", action="append_const", const="lxc")

    # --openstack
    #   Use openstack cloud account
    parser.addoption("--openstack", dest="cloud_type", action="append_const", const="openstack")

    # --slow --include-slow
    #   Include tests marked slow in test run
    parser.addoption("--slow", dest="include_slow", action="store_true")
    parser.addoption("--include-slow", dest="include_slow", action="store_true")

    # --launchpad-vm-id
    #   Assign the vm matching this id to vm pool (used to pipe vm_id from resource creation to tests)
    parser.addoption("--launchpad-vm-id", dest="vm_id", action="store")

    # --fail-on-error
    #   Raise exception on errors observed in the system log
    parser.addoption("--fail-on-error", action="store_true")


@pytest.fixture(scope='session', autouse=True)
def cloud_host(request):
    """Fixture that returns --cloud-host option value"""
    return request.config.getoption("--cloud-host")

@pytest.fixture(scope='session', autouse=True)
def confd_host(request):
    """Fixture that returns --confd-host option value"""
    return request.config.getoption("--confd-host")

@pytest.fixture(scope='session', autouse=True)
def launchpad_vm_id(request):
    """Fixture that returns --launchpad-vm-id option value"""
    return request.config.getoption("--launchpad-vm-id")

@pytest.fixture(scope='module', autouse=True)
def logger(request):
    """Fixture that returns a logger that can be used within a module"""
    logger = logging.getLogger(request.module.__name__)
    logging.basicConfig(format='%(asctime)-15s %(levelname)s %(message)s')
    logging.getLogger().setLevel(logging.DEBUG if request.config.option.verbose else logging.INFO)
    return logger

@pytest.fixture(scope='session', autouse=True)
def mgmt_session(request, confd_host, session_type, cloud_type):
    """Fixture that returns mgmt_session to be used

    # Note: cloud_type's exists in this fixture to ensure it appears in test item's _genid

    Arguments:
        confd_host    - host on which confd is running
        session_type  - communication protocol to use [restconf, netconf]
        cloud_type    - cloud account type to use [lxc, openstack]
    """
    if session_type == 'netconf':
        mgmt_session = rift.auto.session.NetconfSession(host=confd_host)
    elif session_type == 'restconf':
        mgmt_session = rift.auto.session.RestconfSession(host=confd_host)

    mgmt_session.connect()
    rift.vcs.vcs.wait_until_system_started(mgmt_session)
    return mgmt_session

@pytest.fixture(scope='session', autouse=True)
def _riftlog_scraper_session(confd_host):
    '''Fixture which returns an instance of rift.auto.log_scraper.RemoteLogScraper to scrape riftlog

    Arguments:
        confd_host - host on which confd is running (mgmt_ip)
    '''
    scraper = rift.auto.log_scraper.RemoteLogScraper('/var/log/rift/rift.log')
    scraper.connect(host=confd_host)
    scraper.scrape(discard=True)

    return scraper

class ExceptionLogged(Exception):
    """ Exception raised when an exception is captured via a log
    """
    pass

@pytest.fixture(scope='function', autouse=True)
def splat_riftlog(request, _riftlog_scraper_session):
    '''Fixture which scrapes riftlog

    @ splat_ : the function returned by this fixture will be run when a
    #          test that uses this fixture fails

    Arguments:
        request - fixture request object
        _riftlog_scraper_session - instance of rift.auto.log_scraper.RemoteLogScraper
    '''
    def on_test_failure():
        '''When a test fails scrape the riftlog and print it'''
        scraped = _riftlog_scraper_session.scrape()
        if scraped:
            print("=== START RIFTLOG ===")
            print(scraped)
            print("=== END RIFTLOG ===")

    def fin_scrape():
        '''At the end of each test scrape the riftlog and discard
        any entries scraped.

        if test is run with --fail-on-error raise an exception if
        an error is encountered in the log.
        '''
        scraped = _riftlog_scraper_session.scrape()
        if scraped and request.config.getoption("--fail-on-error"):
            print("=== START RIFTLOG ===")
            print(scraped)
            print("=== END RIFTLOG ===")
            raise ExceptionLogged("Exception witnessed in riftlog")

    request.addfinalizer(fin_scrape)
    return on_test_failure


@pytest.fixture(scope='session', autouse=True)
def test_iteration(request, iteration):
    """Fixture that returns the current iteration in a repeated test"""
    return iteration

def pytest_generate_tests(metafunc):
    """pytest hook
    Allows additional tests to be generated from the results of test discovery

    Arguments:
        metafunc  - object which describes a test function discovered by pytest
    """
    if 'session_type' in metafunc.fixturenames:
        if metafunc.config.option.session_type:
            metafunc.parametrize(
                    "session_type",
                    metafunc.config.option.session_type,
                    scope="session")
        else:
            # Default to use netconf because it is currently more stable
            metafunc.parametrize(
                    "session_type",
                    ['netconf'],
                    scope="session")

    if 'cloud_type' in metafunc.fixturenames:
        if metafunc.config.option.cloud_type:
            metafunc.parametrize(
                    "cloud_type",
                    metafunc.config.option.cloud_type,
                    scope="session")
        else:
            # Default to use lxc
            metafunc.parametrize(
                    "cloud_type",
                    ['lxc'],
                    scope='session')

    if 'iteration' in metafunc.fixturenames:
        iterations = range(int(metafunc.config.option.repeat))
        metafunc.parametrize("iteration", iterations, scope="session")

@pytest.mark.hookwrapper
def pytest_pyfunc_call(pyfuncitem):
    """pytest hook
    Allows additional steps to be added surrounding the calling of a test

    Arguments:
        pyfuncitem - test item that is being called
    """
    test_outcome = yield

    try:
        result = test_outcome.get_result()
    except:
        # Test threw an exception (i.e. failed)
        # run all 'splat_' methods attached to the test
        # logging that occurs during a splat will be added to the captured log (unlike a finalizer)
        for funcarg in pyfuncitem.funcargs:
            if funcarg.startswith('splat_'):
                pyfuncitem.funcargs[funcarg]()

def pytest_runtest_makereport(item, call):
    """pytest hook
    Allows additions to be made to the reporting at the end of each test item

    Arguments:
        item - test item that was last run
        call - Result or Exception info of a test function invocation.
    """
    if "incremental" in item.keywords:
        if call.excinfo is not None:
            if call.excinfo.type == _pytest.runner.Skipped:
                return
            marked_xfail = item.get_marker('xfail')
            marked_teardown = item.get_marker('teardown')
            if marked_xfail or marked_teardown:
                return
            parent = item.parent
            parent._previousfailed = item

def pytest_runtest_setup(item):
    """pytest hook
    Allows for additions to be made to each test item during its setup

    Arugments:
        item - test item that is about to run
    """
    if "incremental" in item.keywords:
        previousfailed = getattr(item.parent, "_previousfailed", None)
        if previousfailed is not None:
            pytest.xfail("previous test failed (%s)" % previousfailed.name)

class TestDependencyError(Exception):
    """ Exception raised when test case dependencies cannot be
    resolved"""
    pass

def pytest_collection_modifyitems(session, config, items):
    """ pytest hook
    Called after collection has been performed, may filter or sort
    test items.

    Arguments:
        session - pytest session. # Note - does not appear to correspond to scope=session
        config  - contents of the pytest configuration
        items   - discovered set of test items
    """

    def sort_items(items, satisfied_deps=None):
        '''Sort items topologically based on their dependencies

        Arguments:
            items - list of items to be considered
            satisfied_deps - set of dependencies that should be considered already satisfied

        Raises:
            TestDependencyError if a cycle is found while resolving dependencies
            TestDependencyError if a required dependency is left unsatisfied

        Returns a sorted list of items
        '''
        dependencies = set([])
        if satisfied_deps:
            setup = set(satisfied_deps)
        else:
            setup = set([])
        edges = OrderedDict()
        groups = {}
        for item in items:
            item_name = item.parent.parent.name + "::" + item.name

            if item.get_marker('incremental'):
                parent_name = item.parent.parent.name
                group_name = parent_name
            else:
                group_name = item_name

            if not group_name in groups:
                groups[group_name] = []

            groups[group_name].append(item)

            m_setup = item.get_marker('setup')
            if m_setup:
                for arg in item.get_marker('setup').args:
                    setup.add(arg)
                    edges[(group_name, arg)] = True

            m_depends = item.get_marker('depends')
            if m_depends:
                for arg in item.get_marker('depends').args:
                    dependencies.add(arg)
                    edges[(arg, group_name)] = True

            m_teardown = item.get_marker('teardown')
            if m_teardown:
                for arg in item.get_marker('teardown').args:
                    dependencies.add(arg)
                    edges[(arg, group_name)] = True
                    edges[('teardown', group_name)] = True
            else:
                # all groups that aren't part of teardown are dependencies of teardown
                edges[(group_name, 'teardown')] = True

        if not edges:
            return []

        missing_deps = dependencies - setup
        if missing_deps != set([]):
            raise TestDependencyError("Failed to satisfy all test dependencies.\nMissing dependencies: {}".format(', '.join(missing_deps)))

        edge_list = ' '.join(itertools.chain(*edges))
        sorted_items = subprocess.check_output('echo "{}" | tsort -'.format(edge_list), shell=True).decode()
        if sorted_items.find('input contains a loop') != -1:
            raise TestDependencyError("Cyclical dependency detected in test sequence")

        sorted_groups = sorted_items.split('\n')

        sorted_items = []
        for group_name in sorted_groups:
            if group_name in groups:
                sorted_items.extend(groups[group_name])

        return sorted_items

    def filter_items(items, filter_value):
        filtered_items = []
        for item in items:
            if item.get_marker("slow") and not item.config.option.include_slow:
                continue
            if filter_value in item._genid:
                filtered_items.append(item)
        return filtered_items

    def repeat_filter(items, config):
        '''Filter items to be repeated using the repeat_keyword and repeat_mark flags

        Arguments:
            items - list of test items to be considered
            config - pytest configuration

        Returns a list of test items to be repeated.
        '''
        keywordexpr = config.option.repeat_keyword
        matchexpr = config.option.repeat_mark

        if not keywordexpr and not matchexpr:
            return []
            
        if keywordexpr.startswith("-"):
            keywordexpr = "not " + keywordexpr[1:]

        selectuntil = False
        if keywordexpr[-1:] == ":":
            selectuntil = True
            keywordexpr = keywordexpr[:-1]

        remaining = []
        for colitem in items:
        
            try:
                if keywordexpr and not _pytest.mark.matchkeyword(colitem, keywordexpr):
                    continue
            except:
                if keywordexpr and not _pytest.mark.skipbykeyword(colitem, keywordexpr):
                    continue

            if selectuntil:
                keywordexpr = None

            if matchexpr:
                if not _pytest.mark.matchmark(colitem, matchexpr):
                    continue
                    
            remaining.append(colitem)

        return remaining

    def generate_global_ordering(items, config):
        '''Create a single global ordering for all tests
        test items are divided into one_shot_items which should be invoked only once
        and repeat_items, which should be invoked on each test iteration

        Global test item ordering:
          one_shot_items - setup
          one_shot_items

          per protocol
            per repeat iteration
              repeat_items - setup
              repeat_items
              repeat_items - teardown
          
          one_shot_items - teardown

        Arguments:
            items - list of test items to be considered
            config - pytest configuration

        Returns:
            globally ordered list of test items
        '''
        ordered_items = []
        iterations = range(int(config.option.repeat))

        repeat_items = repeat_filter(items, config)
        if not repeat_items:
            # If there are no items specified for repeat the set of items to repeat is all items
            one_shot_itemlist = []
            repeat_items = items
        else:
            repeat_items_set = set(repeat_items)
            one_shot_itemlist = [item for item in items if item not in repeat_items_set]

        need_deps = False
        for item in items:
            for marker_type in ['setup','depends','teardown']:
                if item.get_marker(marker_type):
                    need_deps = True
                    break

            if need_deps:
                break


        satisfied_deps = set()
        if one_shot_itemlist:
            # make sure to use only using one implementation of the one_shot_items as they aren't being repeated
            if need_deps:
                for item_type in ['netconf-lxc','restconf-lxc','netconf-openstack','restconf-openstack']:
                    one_shot_items = sort_items(filter_items(one_shot_itemlist, '{}-0'.format(item_type)))
                    if one_shot_items:
                        break
            else:
                for item_type in ['netconf-lxc','restconf-lxc','netconf-openstack','restconf-openstack']:
                    one_shot_items = filter_items(one_shot_itemlist, '{}-0'.format(item_type))
                    if one_shot_items:
                        break

            item_index = 0
            while item_index < len(one_shot_items):
                item = one_shot_items[item_index]
                setup = item.get_marker('setup')
                if setup:
                    [satisfied_deps.add(arg) for arg in setup.args]
                if item.get_marker('teardown'):
                    break
                item_index += 1

            ordered_items.extend(one_shot_items[:item_index])

        if need_deps:
            for iteration in iterations:
                ordered_items.extend(sort_items(filter_items(repeat_items, 'netconf-lxc-{}'.format(iteration)), satisfied_deps))
                ordered_items.extend(sort_items(filter_items(repeat_items, 'restconf-lxc-{}'.format(iteration)), satisfied_deps))
                ordered_items.extend(sort_items(filter_items(repeat_items, 'netconf-openstack-{}'.format(iteration)), satisfied_deps))
                ordered_items.extend(sort_items(filter_items(repeat_items, 'restconf-openstack-{}'.format(iteration)), satisfied_deps))
        else:
            for iteration in iterations:
                ordered_items.extend(filter_items(repeat_items, 'netconf-lxc-{}'.format(iteration)))
                ordered_items.extend(filter_items(repeat_items, 'restconf-lxc-{}'.format(iteration)))
                ordered_items.extend(filter_items(repeat_items, 'netconf-openstack-{}'.format(iteration)))
                ordered_items.extend(filter_items(repeat_items, 'restconf-openstack-{}'.format(iteration)))

        if one_shot_itemlist and one_shot_items:
            ordered_items.extend(one_shot_items[item_index:])

        return ordered_items

    final_items = generate_global_ordering(items, config)
    deselected = list(set(items) - set(final_items))
    config.hook.pytest_deselected(items=deselected)
    items[:] = final_items

