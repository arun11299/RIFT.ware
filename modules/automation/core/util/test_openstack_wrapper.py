#!/usr/bin/env python3

# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import argparse
import logging
import os
import re
import signal
import subprocess
import sys
import time
import uuid

import rw_peas
import rwlogger

from gi.repository import (
    RwMcYang,
    RwcalYang,
    RwTypes,
)

class ValidationError(Exception):
    '''Thrown if success of the bootstrap process is not verified'''
    pass

def wait_for_image_state(account, image_id, state, timeout=300):
    state=state.lower()
    current_state = 'unknown'
    while current_state != state:
        rc, image_info = cal.get_image(account, image_id)
        current_state = image_info.state.lower()
        if current_state in ['failed']:
           raise ValidationError('Image [{}] entered failed state while waiting for state [{}]'.format(image_id, state))
        if current_state != state:
            time.sleep(1)
    if current_state != state:
        logger.error('Image still in state [{}] after [{}] seconds'.format(current_state, timeout))
        raise TimeoutError('Image [{}] failed to reach state [{}] within timeout [{}]'.format(image_id, state, timeout))
    return image_info

def wait_for_vdu_state(account, vdu_id, state, timeout=300):
    state = state.lower()
    current_state = 'unknown'
    while current_state != state:
        rc, vdu_info = cal.get_vdu(account, vdu_id)
        print(vdu_info)
        assert rc == RwTypes.RwStatus.SUCCESS
        current_state = vdu_info.state.lower()
        if current_state in ['failed']:
           raise ValidationError('VM [{}] entered failed state while waiting for state [{}]'.format(vdu_id, state))
        if current_state != state:
            time.sleep(1)
    if current_state != state:
        raise TimeoutError('VM [{}] failed to reach state [{}] within timeout [{}]'.format(vdu_id, state, timeout))
    return vdu_info

def parse_known_args(argv=sys.argv[1:]):
    """Create a parser and parse system test arguments

    Arguments:
        argv - list of args to parse

    Returns:
        A tuple containg two elements
            A list containg the unparsed portion of argv
            A namespace object populated with parsed args as attributes
    """

    parser = argparse.ArgumentParser()
    parser.add_argument(
            '-c', '--cloud-host',
            help='address of the openstack host')

    parser.add_argument(
            '--dts-cmd',
            help='command to setup a trace of a dts client')

    parser.add_argument(
            '-n', '--num-vm-resources',
            default=0,
            type=int,
            help='This flag indicates the number of additional vm resources to create')

    parser.add_argument(
            '--systest-script',
            help='system test wrapper script')

    parser.add_argument(
            '--systest-args',
            help='system test wrapper script')

    parser.add_argument(
            '--system-script',
            help='script to bring up system')

    parser.add_argument(
            '--system-args',
            help='arguments to the system script')

    parser.add_argument(
            '--test-script',
            help='script to test the system')

    parser.add_argument(
            '--test-args',
            help='arguments to the test script')

    parser.add_argument(
            '--up-cmd',
            help='command to run to wait until system is up')

    parser.add_argument(
            '-v', '--verbose',
            action='store_true',
            help='This flag sets the logging level to DEBUG')

    parser.add_argument(
            '--wait',
            action='store_true',
            help='wait for interrupt after tests complete')

    return parser.parse_known_args(argv)

if __name__ == '__main__':
    (args, unparsed_args) = parse_known_args()

    flavor_id = None
    lp_vdu_id = None
    mc_vdu_id = None
    image_id = None

    vdu_resources = []

    def handle_term_signal(_signo, _stack_frame):
        sys.exit(2)

    signal.signal(signal.SIGINT, handle_term_signal)
    signal.signal(signal.SIGTERM, handle_term_signal)

    test_execution_rc = 1

    try:
        logging_level = logging.DEBUG if args.verbose else logging.INFO
        logging.basicConfig(level=logging_level)
        logger = logging.getLogger(__name__)
        logger.setLevel(logging_level)

        def get_cal_interface():
            """Get an instance of the rw.cal interface

            Load an instance of the rw.cal plugin via libpeas
            and returns the interface to that plugin instance

            Returns:
                rw.cal interface to created rw.cal instance
            """
            plugin = rw_peas.PeasPlugin('rwcal_openstack', 'RwCal-1.0')
            engine, info, extension = plugin()
            cal = plugin.get_interface("Cloud")
            rwloggerctx = rwlogger.RwLog.Ctx.new("Cal-Log")
            rc = cal.init(rwloggerctx)
            assert rc == RwTypes.RwStatus.SUCCESS
            return cal

        logger.debug("Initializing CAL Interface")
        cal = get_cal_interface()

        username='pluto'
        password='mypasswd'
        auth_url='http://{cloud_host}:5000/v3/'.format(cloud_host=args.cloud_host)
        project_name='demo'
        mgmt_network='private'

        account = RwcalYang.CloudAccount.from_dict({
                'name':'rift.auto.openstack',
                'account_type':'openstack',
                'openstack':{
                  'key':username,
                  'secret':password,
                  'auth_url':auth_url,
                  'tenant':project_name,
                  'mgmt_network':mgmt_network}})

        # Heavy handed fix for resources not being cleaned up [RIFT-10488, RIFT-10611]
        rc, vdulist = cal.get_vdu_list(account)
        for vduinfo in vdulist.vdu_info_list:
            if vduinfo.name in ["rift.auto.mission_control", "rift.auto.launchpad"]:
                cal.delete_vdu(account, vduinfo.vdu_id)

        flavor_name = 'rift.auto.flavor.{}'.format(str(uuid.uuid4()))
        flavor = RwcalYang.FlavorInfoItem.from_dict({
            'name':flavor_name,
            'vm_flavor':{
                'memory_mb':8192, # 8 GB
                'vcpu_count':4,
                'storage_gb':40, # 40 GB
            }
        })

        logger.debug("Creating VM Flavor")
        rc, flavor_id = cal.create_flavor(account, flavor)
        assert rc == RwTypes.RwStatus.SUCCESS

        image = RwcalYang.ImageInfoItem.from_dict({
            'name':'rift.auto.image',
            'location':'/net/sharedfiles/home1/common/vm/rift-root-latest.qcow2',
            'disk_format':'qcow2',
            'container_format':'bare'})

        logger.debug("Uploading VM Image")
        rc, image_id = cal.create_image(account, image)
        assert rc == RwTypes.RwStatus.SUCCESS
        image_info = wait_for_image_state(account, image_id, 'active')

        mc_id = str(uuid.uuid4())
        mc_userdata = """#cloud-config

    runcmd:
     - /usr/rift/scripts/cloud/enable_lab
    """

        mc_vdu_info = RwcalYang.VDUInitParams.from_dict({
            'name':'rift.auto.mission_control',
            'node_id':mc_id,
            'flavor_id':str(flavor_id),
            'image_id':image_id,
            'allocate_public_address':False,
            'vdu_init':{
                'userdata':mc_userdata}
        })


        logger.debug("Creating Mission Control VM")
        rc, mc_vdu_id = cal.create_vdu(account, mc_vdu_info)
        assert rc == RwTypes.RwStatus.SUCCESS
        # OPTIMIZE: We only really need the mission control vm's management ip to proceed
        mc_vdu_info = wait_for_vdu_state(account, mc_vdu_id, 'active')
        logger.debug("Mission Control VM Active!")

        class VDUResource:
            def __init__(self, vdu_id, vdu_info):
                self.vdu_id = vdu_id
                self.vdu_info = vdu_info
                self.accessible = False

        def create_vdu_resource(mc_vdu_info, vdu_name):
            vdu_id = str(uuid.uuid4())
            vdu_userdata = """#cloud-config

        salt_minion:
          conf:
            master: {mgmt_ip}
            id: {vdu_id}
            acceptance_wait_time: 1
            recon_default: 100
            recon_max: 1000
            recon_randomize: False
            log_level: debug

        runcmd:
         - echo Sleeping for 5 seconds and attempting to start minion
         - sleep 5
         - /bin/systemctl start salt-minion.service

        runcmd:
         - /usr/rift/scripts/cloud/enable_lab
        """.format(mgmt_ip=mc_vdu_info.management_ip, vdu_id=vdu_id)

            vdu_info = RwcalYang.VDUInitParams.from_dict({
                'name':vdu_name,
                'node_id':vdu_id,
                'flavor_id':str(flavor_id),
                'image_id':image_id,
                'allocate_public_address':False,
                'vdu_init':{'userdata':vdu_userdata},
            })

            logger.debug("Creating vdu resource {}".format(vdu_name))
            rc, vdu_id = cal.create_vdu(account, vdu_info)
            return VDUResource(vdu_id, vdu_info)

        vdu_resources.append(VDUResource(mc_vdu_id, mc_vdu_info))
        launchpad_resource = create_vdu_resource(mc_vdu_info, 'rift.auto.launchpad')
        vdu_resources.append(launchpad_resource)

        for index in range(args.num_vm_resources):
            resource = create_vdu_resource(mc_vdu_info, 'rift.auto.resource.{}'.format(index))
            vdu_resources.append(resource)

        for vdu in vdu_resources:
            vdu.vdu_info = wait_for_vdu_state(account, vdu.vdu_id, 'active')

        all_vdu_resources_accessible = False
        start = time.time()
        elapsed = 0
        timeout = 600
        check_host_cmd = 'ssh -o UserKnownHostsFile=/dev/null -o BatchMode=yes -o StrictHostKeyChecking=no -- {mgmt_ip} ls'
        while elapsed < timeout:
            all_vdu_resources_accessible = True

            for vdu in vdu_resources:
                if vdu.accessible:
                    continue
                rc = subprocess.call(check_host_cmd.format(mgmt_ip=vdu.vdu_info.management_ip), shell=True)
                if rc != 0:
                    all_vdu_resources_accessible = False
                    break
                else:
                    logger.info("Successfully connected to %s", vdu.vdu_info.name)
                    vdu.accessible = True


            if all_vdu_resources_accessible:
                break

            time.sleep(5)
            elapsed = time.time() - start

        if not all_vdu_resources_accessible:
            raise ValidationError("Failed to verify all VM resources started")

        # Potential work around for RIFT-10299
        time.sleep(400)

        systemtest_cmd = (
                '{systest_script} --up_cmd "{up_cmd}" {systest_args} --system_cmd "ssh -o UserKnownHostsFile=/dev/null -o BatchMode=yes -o StrictHostKeyChecking=no -- {mgmt_ip} sudo {rift_root}/rift-shell -e -r -- {system_script} {system_args}" --test_cmd "{test_script} --confd-host {mgmt_ip} {test_args} --launchpad-vm-id {launchpad_vm_id}"'
                ).format(
                    rift_root="${RIFT_ROOT}",
                    systest_script=args.systest_script,
                    up_cmd=args.up_cmd.replace('CONFD_HOST', mc_vdu_info.management_ip),
                    systest_args=args.systest_args,
                    system_script=args.system_script,
                    system_args=args.system_args,
                    test_script=args.test_script,
                    test_args=args.test_args,
                    mgmt_ip=mc_vdu_info.management_ip,
                    launchpad_vm_id=launchpad_resource.vdu_id)

        print('Executing Systemtest with command: {}'.format(systemtest_cmd))
        test_execution_rc = subprocess.call(systemtest_cmd, shell=True)

        if args.wait:
            # Wait for signal to cleanup
            signal.pause()

    finally:
        for vdu in vdu_resources:
            logger.debug("deleting vdu {}".format(vdu.vdu_info.name))
            cal.delete_vdu(account, vdu.vdu_id)

        if image_id:
            logger.debug("Deleting VM Image")
            cal.delete_image(account, image_id)

        # Try to clean up dynamic resources (whose names should be UUIDs)
        hex_group_re = '[0-9a-fA-F]'
        uuid_re = '^%(hex)s{8}-%(hex)s{4}-%(hex)s{4}-%(hex)s{4}-%(hex)s{12}$' % {'hex':hex_group_re}
        rc, vdu_list = cal.get_vdu_list(account)
        for vdu in vdu_list.vdu_info_list:
            if not re.match(uuid_re, vdu.name):
                continue
            cal.delete_vdu(account, vdu.vdu_id)

        if flavor_id:
            logger.debug("Deleting VM Flavor")
            cal.delete_flavor(account, flavor_id)

    exit(test_execution_rc)
