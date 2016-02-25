/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
/**
 * Created by onvelocity on 11/23/15.
 */

'use strict';

import utils from '../../utils'
import DescriptorModel from '../DescriptorModel'
import DescriptorModelFactory from '../DescriptorModelFactory'

export default class VirtualNetworkFunction extends DescriptorModel {

	static get type() {
		return 'vnfd';
	}

	static get className() {
		return 'VirtualNetworkFunction';
	}

	constructor(model, parent) {
		super(model, parent);
		this.type = 'vnfd';
		this.className = 'VirtualNetworkFunction';
	}

	get vdu() {
		const list = this.model.vdu || (this.model.vdu = []);
		return list.map(d => DescriptorModelFactory.newVirtualDeploymentUnit(d, this));
	}

	set vdu(obj) {
		const onAddNew = (vdu) => {
			vdu.model['name'] = this.vdu.map(utils.suffixAsInteger('name')).reduce(utils.toBiggestValue, this.vdu.length);
		};
		this.updateModelList('vdu', obj, DescriptorModelFactory.VirtualDeploymentUnit, onAddNew);
	}

	get vld() {
		const list = this.model['internal-vld'] || (this.model['internal-vld'] = []);
		return list.map(d => DescriptorModelFactory.newInternalVirtualLink(d, this));
	}

	set vld(obj) {
		const onAddNew = (vld) => {
			vld.model['name'] = this.vld.map(utils.suffixAsInteger('name')).reduce(utils.toBiggestValue, this.vdu.length);
		};
		this.updateModelList('internal-vld', obj, DescriptorModelFactory.InternalVirtualLink, onAddNew);
	}

	createVld() {
		return this.vld = DescriptorModelFactory.newInternalVirtualLink({}, this);
	}

	removeVld(vld) {
		this.removeModelListItem('vld', vld);
	}

	get connectionPoint() {
		const list = this.model['connection-point'] || (this.model['connection-point'] = []);
		return list.map(d => DescriptorModelFactory.newConnectionPoint(d, this));
	}

	set connectionPoint(obj) {
		this.updateModelList('connection-point', obj, DescriptorModelFactory.ConnectionPoint);
	}

	get connectors() {
		return this.connectionPoint;
	}

	remove() {
		if (this.parent) {
			this.parent.removeConstituentVnfd(this);
			this.connectors.forEach(cpc => this.parent.removeAnyConnectionsForConnector(cpc));
		}
	}

	removeAnyConnectionsForConnector(cpc) {
		debugger;
		this.vld.forEach(vld => vld.removeInternalConnectionPointRefForId(cpc.id));
	}


}

