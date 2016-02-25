
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import time
import threading
import logging
import rift.rwcal.openstack as openstack_drv
import rw_status
import rwlogger
from gi.repository import (
    GObject,
    RwCal,
    RwTypes,
    RwcalYang)
import os, subprocess

PREPARE_VM_CMD = "prepare_vm.py --auth_url {auth_url} --username {username} --password {password} --tenant_name {tenant_name} --mgmt_network {mgmt_network} --server_id {server_id} --port_metadata"

logger = logging.getLogger('rwcal.openstack')
logger.setLevel(logging.DEBUG)

rwstatus = rw_status.rwstatus_from_exc_map({ IndexError: RwTypes.RwStatus.NOTFOUND,
                                             KeyError: RwTypes.RwStatus.NOTFOUND,
                                             NotImplementedError: RwTypes.RwStatus.NOT_IMPLEMENTED,})


class RwcalOpenstackPlugin(GObject.Object, RwCal.Cloud):
    """This class implements the CAL VALA methods for openstack."""

    def __init__(self):
        GObject.Object.__init__(self)
        self._driver_class = openstack_drv.OpenstackDriver


    def _get_driver(self, account):
        try:
            drv = self._driver_class(username     = account.openstack.key,
                                  password     = account.openstack.secret,
                                  auth_url     = account.openstack.auth_url,
                                  tenant_name  = account.openstack.tenant,
                                  mgmt_network = account.openstack.mgmt_network)
        except Exception as e:
            logger.error("RwcalOpenstackPlugin: OpenstackDriver init failed. Exception: %s" %(str(e)))
            raise

        return drv


    @rwstatus
    def do_init(self, rwlog_ctx):
        if not any(isinstance(h, rwlogger.RwLogger) for h in logger.handlers):
            logger.addHandler(rwlogger.RwLogger(category="rwcal-openstack",
                                                log_hdl=rwlog_ctx,))

    @rwstatus(ret_on_failure=[None])
    def do_validate_cloud_creds(self, account):
        """
        Validates the cloud account credentials for the specified account.
        Performs an access to the resources using Keystone API. If creds
        are not valid, returns an error code & reason string
        Arguments:
            account - a cloud account to validate

        Returns:
            Validation Code and Details String
        """
        status = RwcalYang.CloudConnectionStatus()

        try:
            drv = self._get_driver(account)
        except Exception as e:
            msg = "RwcalOpenstackPlugin: OpenstackDriver connection failed. Exception: %s" %(str(e))
            logger.error(msg)
            status.status = "failure"
            status.details = msg
            return status

        try:
            drv.validate_account_creds()
        except openstack_drv.ValidationError as e:
            logger.error("RwcalOpenstackPlugin: OpenstackDriver credential validation failed. Exception: %s", str(e))
            status.status = "failure"
            status.details = "Invalid Credentials: %s" % str(e)
            return status

        status.status = "success"
        status.details = "Connection was successful"
        return status

    @rwstatus(ret_on_failure=[""])
    def do_get_management_network(self, account):
        """
        Returns the management network associated with the specified account.
        Arguments:
            account - a cloud account

        Returns: 
            The management network
        """
        return account.openstack.mgmt_network

    @rwstatus(ret_on_failure=[""])
    def do_create_tenant(self, account, name):
        """Create a new tenant.

        Arguments:
            account - a cloud account
            name - name of the tenant

        Returns:
            The tenant id
        """
        raise NotImplementedError
    
    @rwstatus
    def do_delete_tenant(self, account, tenant_id):
        """delete a tenant.

        Arguments:
            account - a cloud account
            tenant_id - id of the tenant
        """
        raise NotImplementedError

    @rwstatus(ret_on_failure=[[]])
    def do_get_tenant_list(self, account):
        """List tenants.

        Arguments:
            account - a cloud account

        Returns:
            List of tenants
        """
        raise NotImplementedError

    @rwstatus(ret_on_failure=[""])
    def do_create_role(self, account, name):
        """Create a new user.

        Arguments:
            account - a cloud account
            name - name of the user

        Returns:
            The user id
        """
        raise NotImplementedError

    @rwstatus
    def do_delete_role(self, account, role_id):
        """Delete a user.

        Arguments:
            account - a cloud account
            role_id - id of the user
        """
        raise NotImplementedError

    @rwstatus(ret_on_failure=[[]])
    def do_get_role_list(self, account):
        """List roles.

        Arguments:
            account - a cloud account

        Returns:
            List of roles
        """
        raise NotImplementedError

    @rwstatus(ret_on_failure=[""])
    def do_create_image(self, account, image):
        """Create an image

        Arguments:
            account - a cloud account
            image - a description of the image to create

        Returns:
            The image id
        """
        try:
            fd = open(image.location, "rb")
        except Exception as e:
            logger.error("Could not open file: %s for upload. Exception received: %s", image.location, str(e))
            raise

        kwargs = {}
        kwargs['name'] = image.name
        
        if image.disk_format:
            kwargs['disk_format'] = image.disk_format
        if image.container_format:
            kwargs['container_format'] = image.container_format

        user_tags = []

        tag_fields = ['checksum']
        for field in tag_fields:
            if image.has_field(field):
                # Ensure that neither field nor field value has : in it.
                assert field.find(":") == -1, "Image user tag key can not contain : character"
                assert getattr(image, field).find(":") == -1, "Image user tag value can not contain : character"
                user_tags.append(field+':'+ getattr(image, field))

        kwargs['tags'] = user_tags

        # Create Image
        image_id = self._get_driver(account).glance_image_create(**kwargs)

        # Upload the Image
        self._get_driver(account).glance_image_upload(image_id, fd)
        return image_id

    @rwstatus
    def do_delete_image(self, account, image_id):
        """Delete a vm image.

        Arguments:
            account - a cloud account
            image_id - id of the image to delete
        """
        self._get_driver(account).glance_image_delete(image_id = image_id)


    @staticmethod
    def _fill_image_info(img_info):
        """Create a GI object from image info dictionary

        Converts image information dictionary object returned by openstack
        driver into Protobuf Gi Object

        Arguments:
            account - a cloud account
            img_info - image information dictionary object from openstack

        Returns:
            The ImageInfoItem
        """
        img = RwcalYang.ImageInfoItem()
        img.name = img_info['name']
        img.id   = img_info['id']

        tag_fields = ['checksum']
        # Look for any properties
        for tag in img_info['tags']:
            if tag.split(":")[0] in tag_fields:
                setattr(img, tag.split(":")[0], tag.split(":")[1])
                
        img.disk_format      = img_info['disk_format']
        img.container_format = img_info['container_format']
        if img_info['status'] == 'active':
            img.state = 'active'
        else:
            img.state = 'inactive'
        return img

    @rwstatus(ret_on_failure=[[]])
    def do_get_image_list(self, account):
        """Return a list of the names of all available images.

        Arguments:
            account - a cloud account

        Returns:
            The the list of images in VimResources object
        """
        response = RwcalYang.VimResources()
        image_list = []
        images = self._get_driver(account).glance_image_list()
        for img in images:
            response.imageinfo_list.append(RwcalOpenstackPlugin._fill_image_info(img))
        return response

    @rwstatus(ret_on_failure=[None])
    def do_get_image(self, account, image_id):
        """Return a image information.

        Arguments:
            account - a cloud account
            image_id - an id of the image

        Returns:
            ImageInfoItem object containing image information.
        """
        image = self._get_driver(account).glance_image_get(image_id)
        return RwcalOpenstackPlugin._fill_image_info(image)

    @rwstatus(ret_on_failure=[""])
    def do_create_vm(self, account, vminfo):
        """Create a new virtual machine.

        Arguments:
            account - a cloud account
            vminfo - information that defines the type of VM to create

        Returns:
            The image id
        """
        kwargs = {}
        kwargs['name']      = vminfo.vm_name
        kwargs['flavor_id'] = vminfo.flavor_id
        kwargs['image_id']  = vminfo.image_id
        
        if vminfo.has_field('cloud_init') and vminfo.cloud_init.has_field('userdata'):
            kwargs['userdata']  = vminfo.cloud_init.userdata
        else:
            kwargs['userdata'] = ''
            
        if account.openstack.security_groups:
            kwargs['security_groups'] = account.openstack.security_groups
        
        port_list = []
        for port in vminfo.port_list:
            port_list.append(port.port_id)

        if port_list:
            kwargs['port_list'] = port_list    

        network_list = []
        for network in vminfo.network_list:
            network_list.append(network.network_id)

        if network_list:
            kwargs['network_list'] = network_list
            
        metadata = {}
        for field in vminfo.user_tags.fields:
            if vminfo.user_tags.has_field(field):
                metadata[field] = getattr(vminfo.user_tags, field)
        kwargs['metadata']  = metadata 

        return self._get_driver(account).nova_server_create(**kwargs)

    @rwstatus
    def do_start_vm(self, account, vm_id):
        """Start an existing virtual machine.

        Arguments:
            account - a cloud account
            vm_id - an id of the VM
        """
        self._get_driver(account).nova_server_start(vm_id)

    @rwstatus
    def do_stop_vm(self, account, vm_id):
        """Stop a running virtual machine.

        Arguments:
            account - a cloud account
            vm_id - an id of the VM
        """
        self._get_driver(account).nova_server_stop(vm_id)

    @rwstatus
    def do_delete_vm(self, account, vm_id):
        """Delete a virtual machine.

        Arguments:
            account - a cloud account
            vm_id - an id of the VM
        """
        self._get_driver(account).nova_server_delete(vm_id)

    @rwstatus
    def do_reboot_vm(self, account, vm_id):
        """Reboot a virtual machine.

        Arguments:
            account - a cloud account
            vm_id - an id of the VM
        """
        self._get_driver(account).nova_server_reboot(vm_id)

    @staticmethod
    def _fill_vm_info(vm_info, mgmt_network):
        """Create a GI object from vm info dictionary

        Converts VM information dictionary object returned by openstack
        driver into Protobuf Gi Object

        Arguments:
            vm_info - VM information from openstack
            mgmt_network - Management network

        Returns:
            Protobuf Gi object for VM
        """
        vm = RwcalYang.VMInfoItem()
        vm.vm_id     = vm_info['id']
        vm.vm_name   = vm_info['name']
        vm.image_id  = vm_info['image']['id']
        vm.flavor_id = vm_info['flavor']['id']
        vm.state     = vm_info['status']
        for network_name, network_info in vm_info['addresses'].items():
            if network_info:
                if network_name == mgmt_network:
                    vm.public_ip = next((item['addr']
                                            for item in network_info
                                            if item['OS-EXT-IPS:type'] == 'floating'),
                                        network_info[0]['addr'])
                    vm.management_ip = network_info[0]['addr']
                else:
                    for interface in network_info:
                        addr = vm.private_ip_list.add()
                        addr.ip_address = interface['addr']
        # Look for any metadata
        for key, value in vm_info['metadata'].items():
            if key in vm.user_tags.fields:
                setattr(vm.user_tags, key, value)
        if 'OS-EXT-SRV-ATTR:host' in vm_info:
            if vm_info['OS-EXT-SRV-ATTR:host'] != None:
                vm.host_name = vm_info['OS-EXT-SRV-ATTR:host']
        if 'OS-EXT-AZ:availability_zone' in vm_info:
            if vm_info['OS-EXT-AZ:availability_zone'] != None:
                vm.availability_zone = vm_info['OS-EXT-AZ:availability_zone']
        return vm

    @rwstatus(ret_on_failure=[[]])
    def do_get_vm_list(self, account):
        """Return a list of the VMs as vala boxed objects

        Arguments:
            account - a cloud account

        Returns:
            List containing VM information
        """
        response = RwcalYang.VimResources()
        vms = self._get_driver(account).nova_server_list()
        for vm in vms:
            response.vminfo_list.append(RwcalOpenstackPlugin._fill_vm_info(vm, account.openstack.mgmt_network))
        return response

    @rwstatus(ret_on_failure=[None])
    def do_get_vm(self, account, id):
        """Return vm information.

        Arguments:
            account - a cloud account
            id - an id for the VM

        Returns:
            VM information
        """
        vm = self._get_driver(account).nova_server_get(id)
        return RwcalOpenstackPlugin._fill_vm_info(vm, account.openstack.mgmt_network)

    @staticmethod
    def _get_guest_epa_specs(guest_epa):
        """
        Returns EPA Specs dictionary for guest_epa attributes
        """
        epa_specs = {}
        if guest_epa.has_field('mempage_size'):
            if guest_epa.mempage_size == 'LARGE':
                epa_specs['hw:mem_page_size'] = 'large'
            elif guest_epa.mempage_size == 'SMALL':
                epa_specs['hw:mem_page_size'] = 'small'
            elif guest_epa.mempage_size == 'SIZE_2MB':
                epa_specs['hw:mem_page_size'] = 2048
            elif guest_epa.mempage_size == 'SIZE_1GB':
                epa_specs['hw:mem_page_size'] = 1048576
            elif guest_epa.mempage_size == 'PREFER_LARGE':
                epa_specs['hw:mem_page_size'] = 'large'
            else:
                assert False, "Unsupported value for mempage_size"
        
        if guest_epa.has_field('cpu_pinning_policy'):
            if guest_epa.cpu_pinning_policy == 'DEDICATED':
                epa_specs['hw:cpu_policy'] = 'dedicated'
            elif guest_epa.cpu_pinning_policy == 'SHARED':
                epa_specs['hw:cpu_policy'] = 'shared'
            elif guest_epa.cpu_pinning_policy == 'ANY':
                pass
            else:
                assert False, "Unsupported value for cpu_pinning_policy"
                
        if guest_epa.has_field('cpu_thread_pinning_policy'):
            if guest_epa.cpu_thread_pinning_policy == 'AVOID':
                epa_specs['hw:cpu_threads_policy'] = 'avoid'
            elif guest_epa.cpu_thread_pinning_policy == 'SEPARATE':
                epa_specs['hw:cpu_threads_policy'] = 'separate'
            elif guest_epa.cpu_thread_pinning_policy == 'ISOLATE':
                epa_specs['hw:cpu_threads_policy'] = 'isolate'
            elif guest_epa.cpu_thread_pinning_policy == 'PREFER':
                epa_specs['hw:cpu_threads_policy'] = 'prefer'
            else:
                assert False, "Unsupported value for cpu_thread_pinning_policy"
                    
        if guest_epa.has_field('trusted_execution'):
            if guest_epa.trusted_execution == True:
                epa_specs['trust:trusted_host'] = 'trusted'
                
        if guest_epa.has_field('numa_node_policy'):
            if guest_epa.numa_node_policy.has_field('node_cnt'):
                epa_specs['hw:numa_nodes'] = guest_epa.numa_node_policy.node_cnt

            if guest_epa.numa_node_policy.has_field('mem_policy'):
                if guest_epa.numa_node_policy.mem_policy == 'STRICT':
                    epa_specs['hw:numa_mempolicy'] = 'strict'
                elif guest_epa.numa_node_policy.mem_policy == 'PREFERRED':
                    epa_specs['hw:numa_mempolicy'] = 'preferred'
                else:
                    assert False, "Unsupported value for num_node_policy.mem_policy"

            if guest_epa.numa_node_policy.has_field('node'):
                for node in guest_epa.numa_node_policy.node:
                    if node.has_field('vcpu') and node.vcpu:
                        epa_specs['hw:numa_cpus.'+str(node.id)] = ','.join([str(j) for j in node.vcpu])
                    if node.memory_mb:
                        epa_specs['hw:numa_mem.'+str(node.id)] = str(node.memory_mb)

        if guest_epa.has_field('pcie_device'):
            pci_devices = []
            for device in guest_epa.pcie_device:
                pci_devices.append(device.device_id +':'+str(device.count))
            epa_specs['pci_passthrough:alias'] = ','.join(pci_devices)

        return epa_specs

    @staticmethod
    def _get_host_epa_specs(guest_epa):
        """
        Returns EPA Specs dictionary for host_epa attributes
        """
        host_epa = {}
        return host_epa

    @staticmethod
    def _get_hypervisor_epa_specs(guest_epa):
        """
        Returns EPA Specs dictionary for hypervisor_epa attributes
        """
        hypervisor_epa = {}
        return hypervisor_epa
    
    @staticmethod
    def _get_vswitch_epa_specs(guest_epa):
        """
        Returns EPA Specs dictionary for vswitch_epa attributes
        """
        vswitch_epa = {}
        return vswitch_epa
    
    @staticmethod
    def _get_epa_specs(flavor):
        """
        Returns epa_specs dictionary based on flavor information
        """
        epa_specs = {}
        if flavor.guest_epa:
            guest_epa = RwcalOpenstackPlugin._get_guest_epa_specs(flavor.guest_epa)
            epa_specs.update(guest_epa)
        if flavor.host_epa:
            host_epa = RwcalOpenstackPlugin._get_host_epa_specs(flavor.host_epa)
            epa_specs.update(host_epa)
        if flavor.hypervisor_epa:
            hypervisor_epa = RwcalOpenstackPlugin._get_hypervisor_epa_specs(flavor.hypervisor_epa)
            epa_specs.update(hypervisor_epa)
        if flavor.vswitch_epa:
            vswitch_epa = RwcalOpenstackPlugin._get_vswitch_epa_specs(flavor.vswitch_epa)
            epa_specs.update(vswitch_epa)
            
        return epa_specs
    
    @rwstatus(ret_on_failure=[""])
    def do_create_flavor(self, account, flavor):
        """Create new flavor.

        Arguments:
            account - a cloud account
            flavor - flavor of the VM

        Returns:
            flavor id
        """
        epa_specs = RwcalOpenstackPlugin._get_epa_specs(flavor)
        return self._get_driver(account).nova_flavor_create(name      = flavor.name,
                                                            ram       = flavor.vm_flavor.memory_mb,
                                                            vcpus     = flavor.vm_flavor.vcpu_count,
                                                            disk      = flavor.vm_flavor.storage_gb,
                                                            epa_specs = epa_specs)


    @rwstatus
    def do_delete_flavor(self, account, flavor_id):
        """Delete flavor.

        Arguments:
            account - a cloud account
            flavor_id - id flavor of the VM
        """
        self._get_driver(account).nova_flavor_delete(flavor_id)

    @staticmethod
    def _fill_epa_attributes(flavor, flavor_info):
        """Helper function to populate the EPA attributes 

        Arguments:
              flavor     : Object with EPA attributes
              flavor_info: A dictionary of flavor_info received from openstack
        Returns:
              None
        """
        getattr(flavor, 'vm_flavor').vcpu_count  = flavor_info['vcpus']
        getattr(flavor, 'vm_flavor').memory_mb   = flavor_info['ram']
        getattr(flavor, 'vm_flavor').storage_gb  = flavor_info['disk']

        ### If extra_specs in flavor_info
        if not 'extra_specs' in flavor_info:
            return

        if 'hw:cpu_policy' in flavor_info['extra_specs']:
            getattr(flavor, 'guest_epa').cpu_pinning_policy = flavor_info['extra_specs']['hw:cpu_policy'].upper()

        if 'hw:cpu_threads_policy' in flavor_info['extra_specs']:
            getattr(flavor, 'guest_epa').cpu_thread_pinning_policy = flavor_info['extra_specs']['hw:cpu_threads_policy'].upper()
            
        if 'hw:mem_page_size' in flavor_info['extra_specs']:
            if flavor_info['extra_specs']['hw:mem_page_size'] == 'large':
                getattr(flavor, 'guest_epa').mempage_size = 'LARGE'
            elif flavor_info['extra_specs']['hw:mem_page_size'] == 'small':
                getattr(flavor, 'guest_epa').mempage_size = 'SMALL'
            elif flavor_info['extra_specs']['hw:mem_page_size'] == 2048:
                getattr(flavor, 'guest_epa').mempage_size = 'SIZE_2MB'
            elif flavor_info['extra_specs']['hw:mem_page_size'] == 1048576:
                getattr(flavor, 'guest_epa').mempage_size = 'SIZE_1GB'
                
        if 'hw:numa_nodes' in flavor_info['extra_specs']:
            getattr(flavor,'guest_epa').numa_node_policy.node_cnt = int(flavor_info['extra_specs']['hw:numa_nodes'])
            for node_id in range(getattr(flavor,'guest_epa').numa_node_policy.node_cnt):
                numa_node = getattr(flavor,'guest_epa').numa_node_policy.node.add()
                numa_node.id = node_id
                if 'hw:numa_cpus.'+str(node_id) in flavor_info['extra_specs']:
                    numa_node.vcpu = [int(x) for x in flavor_info['extra_specs']['hw:numa_cpus.'+str(node_id)].split(',')]
                if 'hw:numa_mem.'+str(node_id) in flavor_info['extra_specs']:
                    numa_node.memory_mb = int(flavor_info['extra_specs']['hw:numa_mem.'+str(node_id)]) 

        if 'hw:numa_mempolicy' in flavor_info['extra_specs']:
            if flavor_info['extra_specs']['hw:numa_mempolicy'] == 'strict':
                getattr(flavor,'guest_epa').numa_node_policy.mem_policy = 'STRICT'
            elif flavor_info['extra_specs']['hw:numa_mempolicy'] == 'preferred':
                getattr(flavor,'guest_epa').numa_node_policy.mem_policy = 'PREFERRED'

                
        if 'trust:trusted_host' in flavor_info['extra_specs']:
            if flavor_info['extra_specs']['trust:trusted_host'] == 'trusted':
                getattr(flavor,'guest_epa').trusted_execution = True
            elif flavor_info['extra_specs']['trust:trusted_host'] == 'untrusted':
                getattr(flavor,'guest_epa').trusted_execution = False

        if 'pci_passthrough:alias' in flavor_info['extra_specs']:
            device_types = flavor_info['extra_specs']['pci_passthrough:alias']
            for device in device_types.split(','):
                dev = getattr(flavor,'guest_epa').pcie_device.add()
                dev.device_id = device.split(':')[0]
                dev.count = int(device.split(':')[1])

            
    @staticmethod
    def _fill_flavor_info(flavor_info):
        """Create a GI object from flavor info dictionary

        Converts Flavor information dictionary object returned by openstack
        driver into Protobuf Gi Object

        Arguments:
            flavor_info: Flavor information from openstack

        Returns:
             Object of class FlavorInfoItem
        """
        flavor = RwcalYang.FlavorInfoItem()
        flavor.name                       = flavor_info['name']
        flavor.id                         = flavor_info['id']
        RwcalOpenstackPlugin._fill_epa_attributes(flavor, flavor_info)
        return flavor
    

    @rwstatus(ret_on_failure=[[]])
    def do_get_flavor_list(self, account):
        """Return flavor information.

        Arguments:
            account - a cloud account

        Returns:
            List of flavors
        """
        response = RwcalYang.VimResources()
        flavors = self._get_driver(account).nova_flavor_list()
        for flv in flavors:
            response.flavorinfo_list.append(RwcalOpenstackPlugin._fill_flavor_info(flv))
        return response

    @rwstatus(ret_on_failure=[None])
    def do_get_flavor(self, account, id):
        """Return flavor information.

        Arguments:
            account - a cloud account
            id - an id for the flavor

        Returns:
            Flavor info item
        """
        flavor = self._get_driver(account).nova_flavor_get(id)
        return RwcalOpenstackPlugin._fill_flavor_info(flavor)

    
    def _fill_network_info(self, network_info, account):
        """Create a GI object from network info dictionary

        Converts Network information dictionary object returned by openstack
        driver into Protobuf Gi Object

        Arguments:
            network_info - Network information from openstack
            account - a cloud account

        Returns:
            Network info item
        """
        network                  = RwcalYang.NetworkInfoItem()
        network.network_name     = network_info['name']
        network.network_id       = network_info['id']
        if ('provider:network_type' in network_info) and (network_info['provider:network_type'] != None):
            network.provider_network.overlay_type = network_info['provider:network_type'].upper()
        if ('provider:segmentation_id' in network_info) and (network_info['provider:segmentation_id']):
            network.provider_network.segmentation_id = network_info['provider:segmentation_id']
        if ('provider:physical_network' in network_info) and (network_info['provider:physical_network']):
            network.provider_network.physical_network = network_info['provider:physical_network'].upper()

        if 'subnets' in network_info and network_info['subnets']:
            subnet_id = network_info['subnets'][0]
            subnet = self._get_driver(account).neutron_subnet_get(subnet_id)
            network.subnet = subnet['cidr']
        return network

    @rwstatus(ret_on_failure=[[]])
    def do_get_network_list(self, account):
        """Return a list of networks

        Arguments:
            account - a cloud account
        
        Returns:
            List of networks
        """
        response = RwcalYang.VimResources()
        networks = self._get_driver(account).neutron_network_list()
        for network in networks:
            response.networkinfo_list.append(self._fill_network_info(network, account))
        return response

    @rwstatus(ret_on_failure=[None])
    def do_get_network(self, account, id):
        """Return a network

        Arguments:
            account - a cloud account
            id - an id for the network

        Returns:
            Network info item
        """
        network = self._get_driver(account).neutron_network_get(id)
        return self._fill_network_info(network, account)

    @rwstatus(ret_on_failure=[""])
    def do_create_network(self, account, network):
        """Create a new network

        Arguments:
            account - a cloud account
            network - Network object

        Returns:
            Network id
        """
        kwargs = {}
        kwargs['name']            = network.network_name
        kwargs['admin_state_up']  = True
        kwargs['external_router'] = False
        kwargs['shared']          = False
        
        if network.has_field('provider_network'):
            if network.provider_network.has_field('physical_network'):
                kwargs['physical_network'] = network.provider_network.physical_network
            if network.provider_network.has_field('overlay_type'):
                kwargs['network_type'] = network.provider_network.overlay_type.lower()
            if network.provider_network.has_field('segmentation_id'):
                kwargs['segmentation_id'] = network.provider_network.segmentation_id
            
        network_id = self._get_driver(account).neutron_network_create(**kwargs)
        self._get_driver(account).neutron_subnet_create(network_id = network_id,
                                                        cidr = network.subnet)
        return network_id

    @rwstatus
    def do_delete_network(self, account, network_id):
        """Delete a network

        Arguments:
            account - a cloud account
            network_id - an id for the network
        """
        self._get_driver(account).neutron_network_delete(network_id)

    @staticmethod
    def _fill_port_info(port_info):
        """Create a GI object from port info dictionary

        Converts Port information dictionary object returned by openstack
        driver into Protobuf Gi Object

        Arguments:
            port_info - Port information from openstack

        Returns:
            Port info item
        """
        port = RwcalYang.PortInfoItem()

        port.port_name  = port_info['name']
        port.port_id    = port_info['id']
        port.network_id = port_info['network_id']
        port.port_state = port_info['status']
        if 'device_id' in port_info:
            port.vm_id = port_info['device_id']
        if 'fixed_ips' in port_info:
            port.ip_address = port_info['fixed_ips'][0]['ip_address']
        return port

    @rwstatus(ret_on_failure=[None])
    def do_get_port(self, account, port_id):
        """Return a port

        Arguments:
            account - a cloud account
            port_id - an id for the port

        Returns:
            Port info item
        """
        port = self._get_driver(account).neutron_port_get(port_id)
        return RwcalOpenstackPlugin._fill_port_info(port)

    @rwstatus(ret_on_failure=[[]])
    def do_get_port_list(self, account):
        """Return a list of ports

        Arguments:
            account - a cloud account

        Returns:
            Port info list
        """
        response = RwcalYang.VimResources()
        ports = self._get_driver(account).neutron_port_list(*{})
        for port in ports:
            response.portinfo_list.append(RwcalOpenstackPlugin._fill_port_info(port))
        return response

    @rwstatus(ret_on_failure=[""])
    def do_create_port(self, account, port):
        """Create a new port

        Arguments:
            account - a cloud account
            port - port object

        Returns:
            Port id
        """
        kwargs = {}
        kwargs['name'] = port.port_name
        kwargs['network_id'] = port.network_id
        kwargs['admin_state_up'] = True
        if port.has_field('vm_id'):
            kwargs['vm_id'] = port.vm_id
        if port.has_field('port_type'):
            kwargs['port_type'] = port.port_type
        else:
            kwargs['port_type'] = "normal"

        return self._get_driver(account).neutron_port_create(**kwargs)

    @rwstatus
    def do_delete_port(self, account, port_id):
        """Delete a port

        Arguments:
            account - a cloud account
            port_id - an id for port
        """
        self._get_driver(account).neutron_port_delete(port_id)
    @rwstatus(ret_on_failure=[""])
    def do_add_host(self, account, host):
        """Add a new host

        Arguments:
            account - a cloud account
            host - a host object

        Returns:
            An id for the host
        """
        raise NotImplementedError

    @rwstatus
    def do_remove_host(self, account, host_id):
        """Remove a host

        Arguments:
            account - a cloud account
            host_id - an id for the host
        """
        raise NotImplementedError

    @rwstatus(ret_on_failure=[None])
    def do_get_host(self, account, host_id):
        """Return a host

        Arguments:
            account - a cloud account
            host_id - an id for host

        Returns:
            Host info item
        """
        raise NotImplementedError

    @rwstatus(ret_on_failure=[[]])
    def do_get_host_list(self, account):
        """Return a list of hosts

        Arguments:
            account - a cloud account

        Returns:
            List of hosts
        """
        raise NotImplementedError

    @staticmethod
    def _fill_connection_point_info(c_point, port_info):
        """Create a GI object for RwcalYang.VDUInfoParams_ConnectionPoints()

        Converts Port information dictionary object returned by openstack
        driver into Protobuf Gi Object  

        Arguments:
            port_info - Port information from openstack
        Returns:
            Protobuf Gi object for RwcalYang.VDUInfoParams_ConnectionPoints
        """
        c_point.name = port_info['name']
        c_point.connection_point_id = port_info['id']
        if ('fixed_ips' in port_info) and (len(port_info['fixed_ips']) >= 1):
            if 'ip_address' in port_info['fixed_ips'][0]:
                c_point.ip_address = port_info['fixed_ips'][0]['ip_address']
        if port_info['status'] == 'ACTIVE':
            c_point.state = 'active'
        else:
            c_point.state = 'inactive'
        if 'network_id' in port_info:    
            c_point.virtual_link_id = port_info['network_id']
        if ('device_id' in port_info) and (port_info['device_id']):
            c_point.vdu_id = port_info['device_id']
        
    @staticmethod
    def _fill_virtual_link_info(network_info, port_list, subnet):
        """Create a GI object for VirtualLinkInfoParams

        Converts Network and Port information dictionary object
        returned by openstack driver into Protobuf Gi Object  

        Arguments:
            network_info - Network information from openstack
            port_list - A list of port information from openstack
            subnet: Subnet information from openstack
        Returns:
            Protobuf Gi object for VirtualLinkInfoParams
        """
        link = RwcalYang.VirtualLinkInfoParams()
        link.name  = network_info['name']
        if network_info['status'] == 'ACTIVE':
            link.state = 'active'
        else:
            link.state = 'inactive'
        link.virtual_link_id = network_info['id']
        for port in port_list:
            if port['device_owner'] == 'compute:None':
                c_point = link.connection_points.add()
                RwcalOpenstackPlugin._fill_connection_point_info(c_point, port)

        if subnet != None:
            link.subnet = subnet['cidr']

        if ('provider:network_type' in network_info) and (network_info['provider:network_type'] != None):
            link.provider_network.overlay_type = network_info['provider:network_type'].upper()
        if ('provider:segmentation_id' in network_info) and (network_info['provider:segmentation_id']):
            link.provider_network.segmentation_id = network_info['provider:segmentation_id']
        if ('provider:physical_network' in network_info) and (network_info['provider:physical_network']):
            link.provider_network.physical_network = network_info['provider:physical_network'].upper()

        return link

    @staticmethod
    def _fill_vdu_info(vm_info, flavor_info, mgmt_network, port_list):
        """Create a GI object for VDUInfoParams

        Converts VM information dictionary object returned by openstack
        driver into Protobuf Gi Object

        Arguments:
            vm_info - VM information from openstack
            flavor_info - VM Flavor information from openstack
            mgmt_network - Management network
            port_list - A list of port information from openstack
        Returns:
            Protobuf Gi object for VDUInfoParams
        """
        vdu = RwcalYang.VDUInfoParams()
        vdu.name = vm_info['name']
        vdu.vdu_id = vm_info['id']
        for network_name, network_info in vm_info['addresses'].items():
            if network_info and network_name == mgmt_network:
                for interface in network_info:
                    if 'OS-EXT-IPS:type' in interface:
                        if interface['OS-EXT-IPS:type'] == 'fixed':
                            vdu.management_ip = interface['addr']
                        elif interface['OS-EXT-IPS:type'] == 'floating':
                            vdu.public_ip = interface['addr']
                
        # Look for any metadata
        for key, value in vm_info['metadata'].items():
            if key == 'node_id':
                vdu.node_id = value
        if ('image' in vm_info) and ('id' in vm_info['image']):
            vdu.image_id = vm_info['image']['id']
        if ('flavor' in vm_info) and ('id' in vm_info['flavor']):
            vdu.flavor_id = vm_info['flavor']['id']

        if vm_info['status'] == 'ACTIVE':
            vdu.state = 'active'
        elif vm_info['status'] == 'ERROR':
            vdu.state = 'failed'
        else:
            vdu.state = 'inactive'
        # Fill the port information
        for port in port_list:
            c_point = vdu.connection_points.add()
            RwcalOpenstackPlugin._fill_connection_point_info(c_point, port)

        if flavor_info is not None:
            RwcalOpenstackPlugin._fill_epa_attributes(vdu, flavor_info)
        return vdu

    @rwstatus(ret_on_failure=[""])
    def do_create_virtual_link(self, account, link_params):
        """Create a new virtual link

        Arguments:
            account     - a cloud account
            link_params - information that defines the type of VDU to create

        Returns:
            The vdu_id
        """
        network = RwcalYang.NetworkInfoItem()
        network.network_name = link_params.name
        network.subnet       = link_params.subnet

        if link_params.provider_network:
            for field in link_params.provider_network.fields:
                if link_params.provider_network.has_field(field):
                    setattr(network.provider_network,
                            field,
                            getattr(link_params.provider_network, field))
        net_id = self.do_create_network(account, network, no_rwstatus=True)
        return net_id
        

    @rwstatus
    def do_delete_virtual_link(self, account, link_id):
        """Delete a virtual link

        Arguments:
            account - a cloud account
            link_id - id for the virtual-link to be deleted

        Returns:
            None
        """
        if not link_id:
            logger.error("Empty link_id during the virtual link deletion")
            raise Exception("Empty link_id during the virtual link deletion")

        port_list = self._get_driver(account).neutron_port_list(**{'network_id': link_id})
        for port in port_list:
            if ((port['device_owner'] == 'compute:None') or (port['device_owner'] == '')):
                self.do_delete_port(account, port['id'], no_rwstatus=True)
        self.do_delete_network(account, link_id, no_rwstatus=True)
        
    @rwstatus(ret_on_failure=[None])
    def do_get_virtual_link(self, account, link_id):
        """Get information about virtual link.

        Arguments:
            account  - a cloud account
            link_id  - id for the virtual-link 

        Returns:
            Object of type RwcalYang.VirtualLinkInfoParams
        """
        if not link_id:
            logger.error("Empty link_id during the virtual link get request")
            raise Exception("Empty link_id during the virtual link get request")
        
        drv = self._get_driver(account)
        network = drv.neutron_network_get(link_id)
        if network:
            port_list = drv.neutron_port_list(**{'network_id': network['id']})
            if 'subnets' in network:
                subnet = drv.neutron_subnet_get(network['subnets'][0])
            else:
                subnet = None
            virtual_link = RwcalOpenstackPlugin._fill_virtual_link_info(network, port_list, subnet)
        else:
            virtual_link = None
        return virtual_link
    
    @rwstatus(ret_on_failure=[None])
    def do_get_virtual_link_list(self, account):
        """Get information about all the virtual links

        Arguments:
            account  - a cloud account

        Returns:
            A list of objects of type RwcalYang.VirtualLinkInfoParams
        """
        vnf_resources = RwcalYang.VNFResources()
        drv = self._get_driver(account)
        networks = drv.neutron_network_list()
        for network in networks:
            port_list = drv.neutron_port_list(**{'network_id': network['id']})
            if ('subnets' in network) and (network['subnets']):
                subnet = drv.neutron_subnet_get(network['subnets'][0])
            else:
                subnet = None
            virtual_link = RwcalOpenstackPlugin._fill_virtual_link_info(network, port_list, subnet)
            vnf_resources.virtual_link_info_list.append(virtual_link)
        return vnf_resources

    def _create_connection_point(self, account, c_point):
        """
        Create a connection point
        Arguments:
           account  - a cloud account
           c_point  - connection_points
        """
        port            = RwcalYang.PortInfoItem()
        port.port_name  = c_point.name
        port.network_id = c_point.virtual_link_id

        if c_point.type_yang == 'VIRTIO':
            port.port_type = 'normal'
        elif c_point.type_yang == 'SR_IOV':
            port.port_type = 'direct'
        else:
            raise NotImplementedError("Port Type: %s not supported" %(c_point.port_type))
        
        port_id = self.do_create_port(account, port, no_rwstatus=True)
        return port_id

    
    @rwstatus(ret_on_failure=[""])
    def do_create_vdu(self, account, vdu_init):
        """Create a new virtual deployment unit

        Arguments:
            account     - a cloud account
            vdu_init  - information about VDU to create (RwcalYang.VDUInitParams)

        Returns:
            The vdu_id
        """
        ### First create required number of ports aka connection points
        port_list = []
        network_list = []
        drv = self._get_driver(account)

        ### If floating_ip is required and we don't have one, better fail before any further allocation
        if vdu_init.has_field('allocate_public_address') and vdu_init.allocate_public_address:
            pool_name = None
            if account.openstack.has_field('floating_ip_pool'):
                pool_name = account.openstack.floating_ip_pool
            floating_ip = drv.nova_floating_ip_create(pool_name)
        else:
            floating_ip = None
        
        ### Create port in mgmt network
        port            = RwcalYang.PortInfoItem()
        port.port_name  = 'mgmt-'+ vdu_init.name
        port.network_id = drv._mgmt_network_id
        port.port_type = 'normal'
        port_id = self.do_create_port(account, port, no_rwstatus=True)
        port_list.append(port_id)

        
        for c_point in vdu_init.connection_points:
            if c_point.virtual_link_id in network_list:
                assert False, "Only one port per network supported. Refer: http://specs.openstack.org/openstack/nova-specs/specs/juno/implemented/nfv-multiple-if-1-net.html"
            else:
                network_list.append(c_point.virtual_link_id)
            port_id = self._create_connection_point(account, c_point)
            port_list.append(port_id)

        ### Now Create VM
        vm                     = RwcalYang.VMInfoItem()
        vm.vm_name             = vdu_init.name
        vm.flavor_id           = vdu_init.flavor_id
        vm.image_id            = vdu_init.image_id
        
        if vdu_init.has_field('vdu_init') and vdu_init.vdu_init.has_field('userdata'):
            vm.cloud_init.userdata = vdu_init.vdu_init.userdata
            
        vm.user_tags.node_id   = vdu_init.node_id;

        for port_id in port_list:
            port = vm.port_list.add()
            port.port_id = port_id
            
        pci_assignement = self.prepare_vpci_metadata(drv, vdu_init)
        if pci_assignement != '':
            vm.user_tags.pci_assignement = pci_assignement

        vm_id = self.do_create_vm(account, vm, no_rwstatus=True)
        self.prepare_vdu_on_boot(account, vm_id, floating_ip)
        return vm_id

    def prepare_vpci_metadata(self, drv, vdu_init):
        pci_assignement = ''
        ### TEF specific metadata creation for
        virtio_vpci = []
        sriov_vpci = []
        virtio_meta = ''
        sriov_meta = ''
        ### For MGMT interface
        if vdu_init.has_field('mgmt_vpci'):
            xx = 'u\''+ drv._mgmt_network_id + '\' :[[u\'' + vdu_init.mgmt_vpci + '\', ' + '\'\']]'
            virtio_vpci.append(xx)

        for c_point in vdu_init.connection_points:
            if c_point.has_field('vpci'):
                if c_point.has_field('vpci') and c_point.type_yang == 'VIRTIO':
                    xx = 'u\''+c_point.virtual_link_id + '\' :[[u\'' + c_point.vpci + '\', ' + '\'\']]'
                    virtio_vpci.append(xx)
                elif c_point.has_field('vpci') and c_point.type_yang == 'SR_IOV':
                    xx = '[u\'' + c_point.vpci + '\', ' + '\'\']'
                    sriov_vpci.append(xx)
                
        if virtio_vpci:
            virtio_meta += ','.join(virtio_vpci)
            
        if sriov_vpci:
            sriov_meta = 'u\'VF\': ['
            sriov_meta += ','.join(sriov_vpci)
            sriov_meta += ']'

        if virtio_meta != '':
            pci_assignement +=  virtio_meta
            pci_assignement += ','
            
        if sriov_meta != '':
            pci_assignement +=  sriov_meta
            
        if pci_assignement != '':
            pci_assignement = '{' + pci_assignement + '}'
            
        return pci_assignement
    

        
    def prepare_vdu_on_boot(self, account, server_id, floating_ip):
        cmd = PREPARE_VM_CMD.format(auth_url     = account.openstack.auth_url,
                                    username     = account.openstack.key,
                                    password     = account.openstack.secret,
                                    tenant_name  = account.openstack.tenant,
                                    mgmt_network = account.openstack.mgmt_network,
                                    server_id    = server_id)
        
        if floating_ip is not None:
            cmd += (" --floating_ip "+ floating_ip.ip)

        exec_path = 'python3 ' + os.path.dirname(openstack_drv.__file__)
        exec_cmd = exec_path+'/'+cmd
        logger.info("Running command: %s" %(exec_cmd))
        subprocess.call(exec_cmd, shell=True)
        
    @rwstatus
    def do_modify_vdu(self, account, vdu_modify):
        """Modify Properties of existing virtual deployment unit

        Arguments:
            account     -  a cloud account
            vdu_modify  -  Information about VDU Modification (RwcalYang.VDUModifyParams)
        """
        ### First create required number of ports aka connection points
        port_list = []
        network_list = []
        for c_point in vdu_modify.connection_points_add:
            if c_point.virtual_link_id in network_list:
                assert False, "Only one port per network supported. Refer: http://specs.openstack.org/openstack/nova-specs/specs/juno/implemented/nfv-multiple-if-1-net.html"
            else:
                network_list.append(c_point.virtual_link_id)
            port_id = self._create_connection_point(account, c_point)
            port_list.append(port_id)

        ### Now add the ports to VM
        for port_id in port_list:
            self._get_driver(account).nova_server_add_port(vdu_modify.vdu_id, port_id)

        ### Delete the requested connection_points
        for c_point in vdu_modify.connection_points_remove:
            self.do_delete_port(account, c_point.connection_point_id, no_rwstatus=True)

        if vdu_modify.has_field('image_id'):
            self._get_driver(account).nova_server_rebuild(vdu_modify.vdu_id, vdu_modify.image_id)
    
        
    @rwstatus
    def do_delete_vdu(self, account, vdu_id):
        """Delete a virtual deployment unit

        Arguments:
            account - a cloud account
            vdu_id  - id for the vdu to be deleted

        Returns:
            None
        """
        if not vdu_id:
            logger.error("empty vdu_id during the vdu deletion")
            return
        drv = self._get_driver(account)

        ### Get list of floating_ips associated with this instance and delete them
        floating_ips = [ f for f in drv.nova_floating_ip_list() if f.instance_id == vdu_id ]
        for f in floating_ips:
            drv.nova_drv.floating_ip_delete(f)

        ### Get list of port on VM and delete them.
        port_list = drv.neutron_port_list(**{'device_id': vdu_id})
        for port in port_list:
            if ((port['device_owner'] == 'compute:None') or (port['device_owner'] == '')):
                self.do_delete_port(account, port['id'], no_rwstatus=True)
        self.do_delete_vm(account, vdu_id, no_rwstatus=True)
                
    
    @rwstatus(ret_on_failure=[None])
    def do_get_vdu(self, account, vdu_id):
        """Get information about a virtual deployment unit.

        Arguments:
            account - a cloud account
            vdu_id  - id for the vdu

        Returns:
            Object of type RwcalYang.VDUInfoParams
        """
        drv = self._get_driver(account)

        ### Get list of ports excluding the one for management network
        port_list = [p for p in drv.neutron_port_list(**{'device_id': vdu_id}) if p['network_id'] != drv.get_mgmt_network_id()]

        vm = drv.nova_server_get(vdu_id)

        flavor_info = None
        if ('flavor' in vm) and ('id' in vm['flavor']):
            try:
                flavor_info = drv.nova_flavor_get(vm['flavor']['id'])
            except Exception as e:
                logger.critical("Exception encountered while attempting to get flavor info for flavor_id: %s. Exception: %s" %(vm['flavor']['id'], str(e)))
                
        return RwcalOpenstackPlugin._fill_vdu_info(vm,
                                                   flavor_info,
                                                   account.openstack.mgmt_network,
                                                   port_list)
        

    @rwstatus(ret_on_failure=[None])
    def do_get_vdu_list(self, account):
        """Get information about all the virtual deployment units

        Arguments:
            account     - a cloud account

        Returns:
            A list of objects of type RwcalYang.VDUInfoParams
        """
        vnf_resources = RwcalYang.VNFResources()
        drv = self._get_driver(account)
        vms = drv.nova_server_list()
        for vm in vms:
            ### Get list of ports excluding one for management network
            port_list = [p for p in drv.neutron_port_list(**{'device_id': vm['id']}) if p['network_id'] != drv.get_mgmt_network_id()]

            flavor_info = None

            if ('flavor' in vm) and ('id' in vm['flavor']):
                try:
                    flavor_info = drv.nova_flavor_get(vm['flavor']['id'])
                except Exception as e:
                    logger.critical("Exception encountered while attempting to get flavor info for flavor_id: %s. Exception: %s" %(vm['flavor']['id'], str(e)))
 
            else:
                flavor_info = None
            vdu = RwcalOpenstackPlugin._fill_vdu_info(vm,
                                                      flavor_info,
                                                      account.openstack.mgmt_network,
                                                      port_list)
            vnf_resources.vdu_info_list.append(vdu)
        return vnf_resources
    
    
