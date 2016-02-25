/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
/**
 * Created by onvelocity on 10/6/15.
 *
 * Model fragments used to construct partially completed models.
 */

'use strict';

import guid from './../guid'
import InstanceCounter from './../InstanceCounter'

export default {
	'vnfd': {
		'description': 'A simple VNF descriptor w/ one VDU',
		'version': '0.1.0',
		'internal-vld': [
			{
				'id': () => guid(),
				'name': () => 'fabric-' + InstanceCounter.count('vnfd.internal-vld'),
				'description': 'Virtual link for internal fabric',
				'type': 'ELAN'
			}
		],
		'connection-point': [
			{
				'name': 'ping-vnfd/cp0',
				'type': 'VPORT'
			},
			{
				'name': 'ping-vnfd/cp1',
				'type': 'VPORT'
			}
		],
		'vdu': [
			{
				'id': () => guid(),
				'name': () => 'vdu-' + InstanceCounter.count('vnfd.vdu'),
				'count': 2,
				'vm-flavor': {
					'vcpu-count': 4,
					'memory-mb': 16384,
					'storage-gb': 16
				},
				'guest-epa': {
					'trusted-execution': true,
					'mempage-size': 'PREFER_LARGE',
					'cpu-pinning-policy': 'DEDICATED',
					'cpu-thread-pinning-policy': 'AVOID',
					'numa-node-policy': {
						'node-cnt': 2,
						'mem-policy': 'PREFERRED',
						'node': [
							{
								'id': 0,
								'vcpu': [
									'0',
									'1'
								],
								'memory-mb': 8192
							},
							{
								'id': 1,
								'vcpu': [
									'2',
									'3'
								],
								'memory-mb': 8192
							}
						]
					}
				},
				'vswitch-epa': {
					'ovs-acceleration': 'DISABLED',
					'ovs-offload': 'DISABLED'
				},
				'hypervisor-epa': {
					'type': 'PREFER_KVM'
				},
				'host-epa': {
					'cpu-model': 'PREFER_SANDYBRIDGE',
					'cpu-arch': 'PREFER_X86_64',
					'cpu-vendor': 'PREFER_INTEL',
					'cpu-socket-count': 'PREFER_TWO',
					'cpu-feature': [
						'PREFER_AES',
						'PREFER_CAT'
					]
				},
				'image': 'rw_openstack.qcow2',
				'internal-connection-point': [
					{
						'id': () => guid(),
						'type': 'VPORT'
					},
					{
						'id': () => guid(),
						'type': 'VPORT'
					}
				],
				'internal-interface': [
					{
						'name': 'eth0',
						'vdu-internal-connection-point-ref': (obj, key, model) => {
							return model.vdu[0]['internal-connection-point'][0].id;
						},
						'virtual-interface': {
							'type': 'VIRTIO'
						}
					},
					{
						'name': 'eth1',
						'vdu-internal-connection-point-ref': (obj, key, model) => {
							return model.vdu[0]['internal-connection-point'][1].id;
						},
						'virtual-interface': {
							'type': 'VIRTIO'
						}
					}
				],
				'external-interface': [
					{
						'name': 'eth0',
						'vnfd-connection-point-ref': 'ping-vnfd/cp0',
						'virtual-interface': {
							'type': 'VIRTIO'
						}
					},
					{
						'name': 'eth1',
						'vnfd-connection-point-ref': 'ping-vnfd/cp1',
						'virtual-interface': {
							'type': 'VIRTIO'
						}
					}
				]
			}
		]
	}
};