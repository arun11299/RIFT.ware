#! /usr/bin/env python
#
#  Demonstrates basic usage of ConfD RESTful API using the Python 'requests'
#  package.  www.python-requests.org
#
#  Assumes examples.confd/rest/basic is running
#

from __future__ import print_function

import requests
import json

BASE_URL = 'http://localhost:8008/api'
AUTH     = ('admin','admin')            # tuple of username, password

# media types
MT_ANY            = '*/*'
MT_BASE           = 'application/vnd.yang.'
MT_YANG_API       = MT_BASE + 'api'
MT_YANG_DATASTORE = MT_BASE + 'datastore'
MT_YANG_DATA      = MT_BASE + 'data'
MT_YANG_OPERATION = MT_BASE + 'operation'

MT_YANG_API_XML        = MT_YANG_API       + '+xml'
MT_YANG_DATASTORE_XML  = MT_YANG_DATASTORE + '+xml'
MT_YANG_DATA_XML       = MT_YANG_DATA      + '+xml'
MT_YANG_OPERATION_XML  = MT_YANG_OPERATION + '+xml'

MT_YANG_API_JSON       = MT_YANG_API       + '+json'
MT_YANG_DATASTORE_JSON = MT_YANG_DATASTORE + '+json'
MT_YANG_DATA_JSON      = MT_YANG_DATA      + '+json'
MT_YANG_OPERATION_JSON = MT_YANG_OPERATION + '+json'

# HTTP response codes
HTTP_RESP_200_OK         = 200
HTTP_RESP_201_CREATE     = 201
HTTP_RESP_204_NO_CONTENT = 204
HTTP_RESP_404_NOT_FOUND  = 404


######################################################################
#  utility functions
######################################################################
def press_enter():
    raw_input('[Press ENTER to continue]')
    print()

def enable_logging():
    import logging
    import httplib

    logging.basicConfig(filename='rest.log', filemode='w')
    logging.getLogger().setLevel(logging.DEBUG)
    requests_log = logging.getLogger('requests.packages.urllib3')
    requests_log.setLevel(logging.DEBUG)
    requests_log.propagate = True

def print_json(json_str):
    print(json.dumps(json.loads(json_str), indent=4))
    print()

    
######################################################################
#  Get of top-level resource using Request
#  examples.confd/rest/basic/README section 1
######################################################################
def get_top_level_w_request():
    print('GET of top-level resource as XML using Request\n')

    resp = requests.get(BASE_URL, auth=AUTH)
    print(resp.content)    # resp.content is bytes; resp.text is unicode string
    press_enter()

    print('GET of top-level resource as JSON using Request\n')

    resp = requests.get(BASE_URL,
                        headers={'Accept' : MT_YANG_API_JSON}, 
                        auth=AUTH)
    print_json(resp.content)
    press_enter()

######################################################################
#  Get of top-level resource using Session
#  examples.confd/rest/basic/README section 1
######################################################################
def get_top_level(session, json=False):
    print('Get of top-level resource as ', end='')
    if json == True:
        print('JSON\n')
        session.headers.update({'Accept' : MT_YANG_API_JSON})
    else:
        print('XML\n')

    resp = session.get(BASE_URL)

    if json == True:
        print_json(resp.content)
        session.headers.update({'Accept' : MT_ANY})
    else:
        print(resp.content)    # resp.content is bytes; resp.text is unicode string

    press_enter()

######################################################################
#  Get of Running config datastore
#  examples.confd/rest/basic/README section 2
######################################################################
def get_running(session, json=False):
    if json == True:
        session.headers.update({'Accept' : MT_YANG_DATASTORE_JSON})

    print('Get of Running config datastore as ', end='')
    if json == True:
        print('JSON\n')
    else:
        print('XML\n')
    press_enter()

    resp = session.get(BASE_URL + '/running')

    if json == True:
        # bug in JSON output encoding (missing commas)
        #print_json(resp.content)
        print(resp.content)
    else:
        print(resp.content)

    press_enter()

    if json == True:
        session.headers.update({'Accept' : MT_ANY})

######################################################################
#  Get of Running config datastore with selectors
#  examples.confd/rest/basic/README section 3
######################################################################
def get_running_w_selector(session, json=False):
    if json == True:
        session.headers.update({'Accept' : MT_YANG_DATASTORE_JSON})

    print('Get of Running config datastore as ', end='')
    if json ==True:
        print('JSON', end='')
    else:
        print('XML', end='')
    print(' with "shallow" selector\n')

    resp = session.get(BASE_URL + '/running',
                       params={'shallow' : ''})

    if json == True:
        # bug in JSON output encoding (missing commas)
        #print_json(resp.content)
        print(resp.content)
    else:
        print(resp.content)

    press_enter()

    print('Get of Running config datastore as ', end='')
    if json == True:
        print('JSON', end='')
    else:
        print('XML', end='')
    print(' with "deep" selector\n')
    press_enter()

    resp = session.get(BASE_URL + '/running',
                       params={'deep' : ''})

    if json == True:
        # bug in JSON output encoding (missing commas)
        #print_json(resp.content)
        print(resp.content)
    else:
        print(resp.content)

    press_enter()

    if json == True:
        session.headers.update({'Accept' : MT_ANY})

######################################################################
#  Delete part of the config and then re-create it
#  examples.confd/rest/basic/README sections 4 & 5
######################################################################
def delete_and_create_resource(session):
    # TODO: add JSON support

    # first get what we're going to delete so that we can re-add it later
    resp = session.get(BASE_URL + '/running/dhcp/subnet/"10.254.239.0/27"')
    saved_entry = resp.content

    # delete
    print('Deleting running/dhcp/subnet/"10.254.239.0/27"...')

    resp = session.delete(BASE_URL + '/running/dhcp/subnet/"10.254.239.0/27"')

    if resp.status_code == HTTP_RESP_204_NO_CONTENT:
        print('Deletion successful\n')
    else:
        print('Deletion failed; code: {0}\n'.format(resp.status_code))

    print('Confirming deletion...')

    resp = session.get(BASE_URL + '/running/dhcp/subnet/"10.254.239.0/27"')

    if resp.status_code == HTTP_RESP_404_NOT_FOUND:
        print('Deletion confirmed\n')
    else:
        print('Deletion did not confirm; code: {0}\n'.format(resp.status_code))

    press_enter()

    # create
    print('Creating running/dhcp/subnet/"10.254.239.0/27"...')

    resp = session.post(BASE_URL + '/running/dhcp',
                        data=saved_entry)

    if resp.status_code == HTTP_RESP_201_CREATE:
        print('Creation successful\n')
    else:
        print('Creation failed; code: {0}\n'.format(resp.status_code))

    print('Confirming creation...')

    resp = session.get(BASE_URL + '/running/dhcp/subnet/"10.254.239.0/27"')

    if resp.status_code == HTTP_RESP_200_OK:
        print('Creation confirmed\n')
    else:
        print('Creation did not confirm; code: {0}\n'.format(resp.status_code))

    press_enter()

######################################################################
#  Modify an existing config item and then re-create it
#  examples.confd/rest/basic/README sections 6 & 9
######################################################################
def modify_resource(session):
    # TODO: add JSON support
    print('Get of running/dhcp/subnet/"10.254.239.0/27"/max-lease-time with "select" query parameter:')

    resp = session.get(BASE_URL + '/running/dhcp/subnet/"10.254.239.0/27"',
                       params={'select' : 'max-lease-time'})

    print(resp.content)

    # modify
    print('Modifying running/dhcp/subnet/"10.254.239.0/27"/max-lease-time...\n')

    new_max_lease_data = '''<subnet>
                                <max-lease-time>3333</max-lease-time>
                            </subnet>'''

    session.patch(BASE_URL + '/running/dhcp/subnet/"10.254.239.0/27"',
                  data=new_max_lease_data)

    # verify
    print('Get of running/dhcp/subnet/"10.254.239.0/27"/max-lease-time with "select" query parameter:')

    resp = session.get(BASE_URL + '/running/dhcp/subnet/"10.254.239.0/27"',
                       params={'select' : 'max-lease-time'})

    print(resp.content)
    press_enter()

    # use rollback operation to restore modified value
    # just apply rollback zero and assume no one has comitted in between...
    print('Using rollback facility to restore max-lease-time...\n')

    rollback_data = '<file>0</file>'

    resp = session.post(BASE_URL + '/running/_rollback',
                        data=rollback_data)

    print('Get of running/dhcp/subnet/"10.254.239.0/27"/max-lease-time with "select" query parameter:')

    resp = session.get(BASE_URL + '/running/dhcp/subnet/"10.254.239.0/27"',
                       params={'select' : 'max-lease-time'})

    print(resp.content)
    press_enter()

######################################################################
#  Replace an existing config item
#  examples.confd/rest/basic/README section 7
######################################################################
def replace_resource(session):
    # TODO: add JSON support
    # get existing data, then modify, then use saved existing data
    # to demonstrate replace
    resp = session.get(BASE_URL + '/running/dhcp/subnet/"10.254.239.0/27"')
    saved_data = resp.content

    # modify
    print('Modifying running/dhcp/subnet/"10.254.239.0/27"/max-lease-time...\n')

    new_max_lease_data = '''<subnet>
                                <max-lease-time>3333</max-lease-time>
                            </subnet>'''

    session.patch(BASE_URL + '/running/dhcp/subnet/"10.254.239.0/27"',
                  data=new_max_lease_data)

    print('Get of running/dhcp/subnet/"10.254.239.0/27"/max-lease-time with "select" query parameter:')

    resp = session.get(BASE_URL + '/running/dhcp/subnet/"10.254.239.0/27"',
                       params={'select' : 'max-lease-time'})

    print(resp.content)
    press_enter()

    print('Replacing running/dhcp/subnet/"10.254.239.0/27"/max-lease-time...\n')

    new_max_lease_data = '''<subnet>
                                <max-lease-time>3333</max-lease-time>
                            </subnet>'''

    session.put(BASE_URL + '/running/dhcp/subnet/"10.254.239.0/27"',
                data=saved_data)

    print('Get of running/dhcp/subnet/"10.254.239.0/27"/max-lease-time with "select" query parameter:')

    resp = session.get(BASE_URL + '/running/dhcp/subnet/"10.254.239.0/27"',
                       params={'select' : 'max-lease-time'})

    print(resp.content)
    press_enter()

######################################################################
#  Evaluate an action
#  examples.confd/rest/basic/README section 8
######################################################################
def evaluate_action(session):
    # TODO: add JSON support when available
    # TODO: add RPC test when available
    print('Triggering set-clock action...\n')

    action_input_data = ''' <set-clock>
                                <clockSettings>1992-12-12T11:11:11</clockSettings>
                                <utc>true</utc>
                                <syncHardwareClock>true</syncHardwareClock>
                            </set-clock>'''

    resp = session.post(BASE_URL + '/running/dhcp/_operations/set-clock',
                        data = action_input_data)

    print('Action output parameters:')
    print(resp.content)
    press_enter()

######################################################################
#  main entry point
######################################################################
def run():
    print()
    
    enable_logging()

    # top-level get using Request
    #get_top_level_w_request()

    # Requests' Session class is a lot more convenient to work with than the
    # Request class.  Session will be used for the remainder of this example.

    # setup the global session
    session = requests.Session()
    session.auth = AUTH

    # top-level get using Session - README section 1
    get_top_level(session)
    get_top_level(session, json=True)
    
    # get running datastore - README section 2
    get_running(session)
    get_running(session, json=True)

    # get running datastore with selectors - README section 3
    get_running_w_selector(session)
    get_running_w_selector(session, json=True)
    
    # delete and create a resource - README sections 4 & 5
    delete_and_create_resource(session)

    # modify an existing resource - README sections 6 & 9 (rollback)
    modify_resource(session)
    
    # replace an existing resource - README section 7
    replace_resource(session)

    # evaluate an action - README section 8
    evaluate_action(session)

######################################################################
if __name__ == '__main__':
    run()
