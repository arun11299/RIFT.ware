#!/usr/bin/env python3

# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#


import sys
import os
import argparse
import uuid
import rift.vcs.component as vcs
from gi.repository import (
    NsdYang,
    VldYang,
    VnfdYang,
    RwNsdYang,
    RwVnfdYang,
    RwYang,
    )

NUM_PING_INSTANCES = 1
MAX_VNF_INSTANCES_PER_NS = 10
use_epa = False
pingcount = NUM_PING_INSTANCES

PING_USERDATA_FILE = '''#cloud-config
password: fedora
chpasswd: { expire: False }
ssh_pwauth: True
runcmd:
  - [ systemctl, daemon-reload ]
  - [ systemctl, enable, ping.service ]
  - [ systemctl, start, --no-block, ping.service ]
  - [ ifup, eth1 ]
'''

PONG_USERDATA_FILE = '''#cloud-config
password: fedora
chpasswd: { expire: False }
ssh_pwauth: True
runcmd:
  - [ systemctl, daemon-reload ]
  - [ systemctl, enable, pong.service ]
  - [ systemctl, start, --no-block, pong.service ]
  - [ ifup, eth1 ]
'''


class UnknownVNFError(Exception):
    pass

class ManoDescriptor(object):
    def __init__(self, name):
        self.name = name
        self.descriptor = None

    def write_to_file(self, module_list, outdir, output_format):
        model = RwYang.Model.create_libncx()
        for module in module_list:
            model.load_module(module)

        if output_format == 'json':
            with open('%s/%s.json' % (outdir, self.name), "w") as fh:
                fh.write(self.descriptor.to_json(model))
        elif output_format.strip() == 'xml':
            with open('%s/%s.xml' % (outdir, self.name), "w") as fh:
                fh.write(self.descriptor.to_xml_v2(model, pretty_print=True))
        else:
            raise("Invalid output format for the descriptor")

class VirtualNetworkFunction(ManoDescriptor):
    def __init__(self, name, instance_count=1):
        self.vnfd_catalog = None
        self.vnfd = None
        self.instance_count = instance_count
        super(VirtualNetworkFunction, self).__init__(name)

    def compose(self, image_name, cloud_init="", endpoint=None, mon_params=[],
                mon_port=8888, mgmt_port=8888, num_vlr_count=1, num_ivlr_count=1,
                num_vms=1, image_md5sum=None):
        self.descriptor = RwVnfdYang.YangData_Vnfd_VnfdCatalog()
        self.id = str(uuid.uuid1())
        vnfd = self.descriptor.vnfd.add()
        vnfd.id = self.id
        vnfd.name = self.name
        vnfd.short_name = self.name
        vnfd.vendor = 'RIFT.io'
        vnfd.description = 'This is an example RIFT.ware VNF'
        vnfd.version = '1.0'

        self.vnfd = vnfd

        internal_vlds = []
        for i in range(num_ivlr_count):
            internal_vld = vnfd.internal_vld.add()
            internal_vld.id = str(uuid.uuid1())
            internal_vld.name = 'fabric%s' % i
            internal_vld.short_name = 'fabric%s' % i
            internal_vld.description = 'Virtual link for internal fabric%s' % i
            internal_vld.type_yang = 'ELAN'
            internal_vlds.append(internal_vld)

        for i in range(num_vlr_count):
            cp = vnfd.connection_point.add()
            cp.type_yang = 'VPORT'
            cp.name = '%s/cp%d' % (self.name, i)

        if endpoint is not None:
            endp = VnfdYang.YangData_Vnfd_VnfdCatalog_Vnfd_HttpEndpoint(
                    path=endpoint, port=mon_port, polling_interval_secs=2
                    )
            vnfd.http_endpoint.append(endp)

        # Monitoring params
        for monp_dict in mon_params:
            monp = VnfdYang.YangData_Vnfd_VnfdCatalog_Vnfd_MonitoringParam.from_dict(monp_dict)
            monp.http_endpoint_ref = endpoint
            vnfd.monitoring_param.append(monp)

        for i in range(num_vms):
            # VDU Specification
            vdu = vnfd.vdu.add()
            vdu.id = str(uuid.uuid1())
            vdu.name = 'iovdu_%s' % i
            vdu.count = 1
            #vdu.mgmt_vpci = '0000:00:20.0'

            # specify the VM flavor
            if use_epa:
                vdu.vm_flavor.vcpu_count = 4
                vdu.vm_flavor.memory_mb = 1024
                vdu.vm_flavor.storage_gb = 4
            else:
                vdu.vm_flavor.vcpu_count = 1
                vdu.vm_flavor.memory_mb = 512
                vdu.vm_flavor.storage_gb = 4

            # Management interface
            mgmt_intf = vnfd.mgmt_interface
            mgmt_intf.vdu_id = vdu.id
            mgmt_intf.port = mgmt_port
            mgmt_intf.dashboard_params.path = "/api/v1/pong/stats"

            vdu.cloud_init = cloud_init

            # sepcify the guest EPA
            if use_epa:
                vdu.guest_epa.trusted_execution = False
                vdu.guest_epa.mempage_size = 'LARGE'
                vdu.guest_epa.cpu_pinning_policy = 'DEDICATED'
                vdu.guest_epa.cpu_thread_pinning_policy = 'PREFER'
                vdu.guest_epa.numa_node_policy.node_cnt = 2
                vdu.guest_epa.numa_node_policy.mem_policy = 'STRICT'

                node = vdu.guest_epa.numa_node_policy.node.add()
                node.id = 0
                node.memory_mb = 512
                node.vcpu = [0, 1]

                node = vdu.guest_epa.numa_node_policy.node.add()
                node.id = 1
                node.memory_mb = 512
                node.vcpu = [2, 3]

                # specify the vswitch EPA
                vdu.vswitch_epa.ovs_acceleration = 'DISABLED'
                vdu.vswitch_epa.ovs_offload = 'DISABLED'

                # Specify the hypervisor EPA
                vdu.hypervisor_epa.type_yang = 'PREFER_KVM'

                # Specify the host EPA
                vdu.host_epa.cpu_model = 'PREFER_SANDYBRIDGE'
                vdu.host_epa.cpu_arch = 'PREFER_X86_64'
                vdu.host_epa.cpu_vendor = 'PREFER_INTEL'
                vdu.host_epa.cpu_socket_count = 'PREFER_TWO'
                vdu.host_epa.cpu_feature = ['PREFER_AES', 'PREFER_CAT']

            vdu.image = image_name
            if image_md5sum is not None:
                vdu.image_checksum = image_md5sum

            for i in range(num_ivlr_count):
                internal_cp = vdu.internal_connection_point.add()
                internal_cp.id = str(uuid.uuid1())
                internal_cp.type_yang = 'VPORT'
                internal_vlds[i].internal_connection_point_ref.append(internal_cp.id)

                internal_interface = vdu.internal_interface.add()
                internal_interface.name = 'fab%d' % i
                internal_interface.vdu_internal_connection_point_ref = internal_cp.id
                internal_interface.virtual_interface.type_yang = 'VIRTIO'

                #internal_interface.virtual_interface.vpci = '0000:00:1%d.0'%i

            for i in range(num_vlr_count):
                external_interface = vdu.external_interface.add()
                external_interface.name = 'eth%d' % i
                external_interface.vnfd_connection_point_ref = '%s/cp%d' % (self.name, i)
                if use_epa:
                    external_interface.virtual_interface.type_yang = 'VIRTIO'
                else:
                    external_interface.virtual_interface.type_yang = 'VIRTIO'
                #external_interface.virtual_interface.vpci = '0000:00:2%d.0'%i

    def write_to_file(self, outdir, output_format):
        dirpath = "%s/%s/vnfd" % (outdir, self.name)
        if not os.path.exists(dirpath):
            os.makedirs(dirpath)
        super(VirtualNetworkFunction, self).write_to_file(['vnfd', 'rw-vnfd'],
                                                          "%s/%s/vnfd" % (outdir, self.name),
                                                          output_format)

class NetworkService(ManoDescriptor):
    def __init__(self, name):
        super(NetworkService, self).__init__(name)

    def ping_config(self):
        suffix = ''
        if use_epa:
            suffix = '_with_epa'
        ping_cfg = r'''
#!/usr/bin/bash

# Rest API config
ping_mgmt_ip='<rw_mgmt_ip>'
ping_mgmt_port=18888

# VNF specific configuration
pong_server_ip='<rw_connection_point_name pong_vnfd%s/cp0>'
ping_rate=5
server_port=5555

# Make rest API calls to configure VNF
curl -D /dev/stdout \
    -H "Accept: application/vnd.yang.data+xml" \
    -H "Content-Type: application/vnd.yang.data+json" \
    -X POST \
    -d "{\"ip\":\"$pong_server_ip\", \"port\":$server_port}" \
    http://${ping_mgmt_ip}:${ping_mgmt_port}/api/v1/ping/server
rc=$?
if [ $rc -ne 0 ]
then
    echo "Failed to set server info for ping!"
    exit $rc
fi

curl -D /dev/stdout \
    -H "Accept: application/vnd.yang.data+xml" \
    -H "Content-Type: application/vnd.yang.data+json" \
    -X POST \
    -d "{\"rate\":$ping_rate}" \
    http://${ping_mgmt_ip}:${ping_mgmt_port}/api/v1/ping/rate
rc=$?
if [ $rc -ne 0 ]
then
    echo "Failed to set ping rate!"
    exit $rc
fi

output=$(curl -D /dev/stdout \
    -H "Accept: application/vnd.yang.data+xml" \
    -H "Content-Type: application/vnd.yang.data+json" \
    -X POST \
    -d "{\"enable\":true}" \
    http://${ping_mgmt_ip}:${ping_mgmt_port}/api/v1/ping/adminstatus/state)
if [[ $output == *"Internal Server Error"* ]]
then
    echo $output
    exit 3
else
    echo $output
fi


exit 0
        ''' % suffix
        return ping_cfg

    def pong_config(self):
        suffix = ''
        if use_epa:
            suffix = '_with_epa'
        pong_cfg = r'''
#!/usr/bin/bash

# Rest API configuration
pong_mgmt_ip='<rw_mgmt_ip>'
pong_mgmt_port=18889
# username=<rw_username>
# password=<rw_password>

# VNF specific configuration
pong_server_ip='<rw_connection_point_name pong_vnfd%s/cp0>'
server_port=5555

# Make Rest API calls to configure VNF
curl -D /dev/stdout \
    -H "Accept: application/vnd.yang.data+xml" \
    -H "Content-Type: application/vnd.yang.data+json" \
    -X POST \
    -d "{\"ip\":\"$pong_server_ip\", \"port\":$server_port}" \
    http://${pong_mgmt_ip}:${pong_mgmt_port}/api/v1/pong/server
rc=$?
if [ $rc -ne 0 ]
then
    echo "Failed to set server(own) info for pong!"
    exit $rc
fi

curl -D /dev/stdout \
    -H "Accept: application/vnd.yang.data+xml" \
    -H "Content-Type: application/vnd.yang.data+json" \
    -X POST \
    -d "{\"enable\":true}" \
    http://${pong_mgmt_ip}:${pong_mgmt_port}/api/v1/pong/adminstatus/state
rc=$?
if [ $rc -ne 0 ]
then
    echo "Failed to enable pong service!"
    exit $rc
fi

exit 0
        ''' % suffix
        return pong_cfg

    def default_config(self, const_vnfd, vnfd):
        vnf_config = const_vnfd.vnf_configuration

        vnf_config.input_params.config_priority = 0
        vnf_config.input_params.config_delay = 0

        # Select "script" configuration
        vnf_config.config_type = 'script'
        vnf_config.script.script_type = 'bash'

        if vnfd.name == 'pong_vnfd' or vnfd.name == 'pong_vnfd_with_epa':
            vnf_config.input_params.config_priority = 1
            # First priority config delay will delay the entire NS config delay
            vnf_config.input_params.config_delay = 60
            vnf_config.config_template = self.pong_config()
        if vnfd.name == 'ping_vnfd' or vnfd.name == 'ping_vnfd_with_epa':
            vnf_config.input_params.config_priority = 2
            vnf_config.config_template = self.ping_config()
            ## Remove this - test only
            ## vnf_config.config_access.mgmt_ip_address = '1.1.1.1'

        print("### TBR ###", vnfd.name, "vng_config = ", vnf_config)

    def compose(self, vnfd_list, cpgroup_list):
        self.descriptor = RwNsdYang.YangData_Nsd_NsdCatalog()
        self.id = str(uuid.uuid1())
        nsd = self.descriptor.nsd.add()
        nsd.id = self.id
        nsd.name = self.name
        nsd.short_name = self.name
        nsd.vendor = 'RIFT.io'
        nsd.description = 'Toy NS'
        nsd.version = '1.0'
        nsd.input_parameter_xpath.append(
                NsdYang.YangData_Nsd_NsdCatalog_Nsd_InputParameterXpath(
                    xpath="/nsd:nsd-catalog/nsd:nsd/nsd:vendor",
                    )
                )

        for cpgroup in cpgroup_list:
            vld = nsd.vld.add()
            vld.id = str(uuid.uuid1())
            vld.name = 'ping_pong_vld' #hard coded
            vld.short_name = vld.name
            vld.vendor = 'RIFT.io'
            vld.description = 'Toy VL'
            vld.version = '1.0'
            vld.type_yang = 'ELAN'

            for cp in cpgroup:
                cpref = vld.vnfd_connection_point_ref.add()
                cpref.member_vnf_index_ref = cp[0]
                cpref.vnfd_id_ref = cp[1]
                cpref.vnfd_connection_point_ref = cp[2]

        member_vnf_index = 1
        for vnfd in vnfd_list:
            for i in range(vnfd.instance_count):
                constituent_vnfd = nsd.constituent_vnfd.add()
                constituent_vnfd.member_vnf_index = member_vnf_index

                constituent_vnfd.vnfd_id_ref = vnfd.descriptor.vnfd[0].id
                self.default_config(constituent_vnfd, vnfd)
                member_vnf_index += 1

    def write_to_file(self, outdir, output_format):
        dirpath = "%s/%s/nsd" % (outdir, self.name)
        if not os.path.exists(dirpath):
            os.makedirs(dirpath)
        super(NetworkService, self).write_to_file(["nsd", "rw-nsd"],
                                                  "%s/%s/nsd" % (outdir, self.name),
                                                  output_format)


def get_ping_mon_params(path):
    return [
            {
                'id': '1',
                'name': 'ping-request-tx-count',
                'json_query_method': "NAMEKEY",
                'value_type': "INT",
                'description': 'no of ping requests',
                'group_tag': 'Group1',
                'widget_type': 'COUNTER',
                'units': 'packets'
                },

            {
                'id': '2',
                'name': 'ping-response-rx-count',
                'json_query_method': "NAMEKEY",
                'value_type': "INT",
                'description': 'no of ping responses',
                'group_tag': 'Group1',
                'widget_type': 'COUNTER',
                'units': 'packets'
                },
            ]


def get_pong_mon_params(path):
    return [
            {
                'id': '1',
                'name': 'ping-request-rx-count',
                'json_query_method': "NAMEKEY",
                'value_type': "INT",
                'description': 'no of ping requests',
                'group_tag': 'Group1',
                'widget_type': 'COUNTER',
                'units': 'packets'
                },

            {
                'id': '2',
                'name': 'ping-response-tx-count',
                'json_query_method': "NAMEKEY",
                'value_type': "INT",
                'description': 'no of ping responses',
                'group_tag': 'Group1',
                'widget_type': 'COUNTER',
                'units': 'packets'
                },
            ]

def generate_ping_pong_descriptors(fmt="json",
                                   write_to_file=False,
                                   out_dir="./",
                                   pingcount=NUM_PING_INSTANCES,
                                   external_vlr_count=1,
                                   internal_vlr_count=0,
                                   num_vnf_vms=1,
                                   ping_md5sum=None,
                                   pong_md5sum=None,
                                   ):
    # List of connection point groups
    # Each connection point group refers to a virtual link
    # the CP group consists of tuples of connection points
    cpgroup_list = []
    for i in range(external_vlr_count):
        cpgroup_list.append([])

    if use_epa:
        suffix = '_with_epa'
    else:
        suffix = '' 

    ping = VirtualNetworkFunction("ping_vnfd%s" % (suffix), pingcount)
    #ping = VirtualNetworkFunction("ping_vnfd", pingcount)
    ping.compose(
            "Fedora-x86_64-20-20131211.1-sda-ping.qcow2",
            PING_USERDATA_FILE,
            "api/v1/ping/stats",
            get_ping_mon_params("api/v1/ping/stats"),
            mon_port=18888,
            mgmt_port=18888,
            num_vlr_count=external_vlr_count,
            num_ivlr_count=internal_vlr_count,
            num_vms=num_vnf_vms,
            image_md5sum=ping_md5sum,
            )

    pong = VirtualNetworkFunction("pong_vnfd%s" % (suffix))
    #pong = VirtualNetworkFunction("pong_vnfd")
    pong.compose(
            "Fedora-x86_64-20-20131211.1-sda-pong.qcow2",
            PONG_USERDATA_FILE,
            "api/v1/pong/stats",
            get_pong_mon_params("api/v1/pong/stats"),
            mon_port=18889,
            mgmt_port=18889,
            num_vlr_count=external_vlr_count,
            num_ivlr_count=internal_vlr_count,
            num_vms=num_vnf_vms,
            image_md5sum=pong_md5sum,
            )

    # Initialize the member VNF index
    member_vnf_index = 1

    # define the connection point groups
    for index, cp_group in enumerate(cpgroup_list):
        desc_id = ping.descriptor.vnfd[0].id
        filename = 'ping_vnfd{}/cp{}'.format(suffix, index)

        for idx in range(pingcount):
            cp_group.append((
                member_vnf_index,
                desc_id,
                filename,
                ))

            member_vnf_index += 1

        desc_id = pong.descriptor.vnfd[0].id
        filename = 'pong_vnfd{}/cp{}'.format(suffix, index)

        cp_group.append((
            member_vnf_index,
            desc_id,
            filename,
            ))

        member_vnf_index += 1

    vnfd_list = [ping, pong]
    nsd_catalog = NetworkService("ping_pong_nsd%s" % (suffix))
    #nsd_catalog = NetworkService("ping_pong_nsd")
    nsd_catalog.compose(vnfd_list, cpgroup_list)

    if write_to_file:
        ping.write_to_file(out_dir, fmt)
        pong.write_to_file(out_dir, fmt)
        nsd_catalog.write_to_file(out_dir, fmt)

    return (ping, pong, nsd_catalog)

def main(argv=sys.argv[1:]):
    global outdir, output_format, use_epa
    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--outdir', default='.')
    parser.add_argument('-f', '--format', default='json')
    parser.add_argument('-e', '--epa', action="store_true", default=False)
    parser.add_argument('-n', '--pingcount', default=NUM_PING_INSTANCES)
    parser.add_argument('--ping-image-md5')
    parser.add_argument('--pong-image-md5')
    args = parser.parse_args()
    outdir = args.outdir
    output_format = args.format
    use_epa = args.epa
    pingcount = args.pingcount

    generate_ping_pong_descriptors(args.format, True, args.outdir, pingcount,
                                   ping_md5sum=args.ping_image_md5, pong_md5sum=args.pong_image_md5)

if __name__ == "__main__":
    main()

