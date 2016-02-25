/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
/**
 * Created by onvelocity on 11/23/15.
 */

'use strict';

import DescriptorModel from '../DescriptorModel'
import DescriptorModelFactory from '../DescriptorModelFactory'

export default class InternalConnectionPointRef extends DescriptorModel {

	static get type() {
		return 'internal-connection-point-ref';
	}

	static get className() {
		return 'InternalConnectionPointRef';
	}

	constructor(model, parent) {
		super(typeof model === 'string' ? {id: model} : model, parent);
		this.className = InternalConnectionPointRef.className;
		this.type = InternalConnectionPointRef.type;
	}

	toString() {
		return this.model.id;
	}

}
