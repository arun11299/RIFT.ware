/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
/**
 * Created by onvelocity on 11/23/15.
 */

'use strict';

import Position from '../../graph/Position'
import DescriptorModel from '../DescriptorModel'
import RspConnectionPointRef from './RspConnectionPointRef'
import VnfdConnectionPointRef from './VnfdConnectionPointRef'

/**
 * A ConnectionPoint is always a child of a VNFD. We use it to build VnfdConnectionPointRef instances. So convenience
 * methods are add to access the fields needed to do that.
 */
export default class ConnectionPoint extends DescriptorModel {

	static get type() {
		return 'connection-point';
	}

	static get className() {
		return 'ConnectionPoint';
	}

	constructor(model, parent) {
		super(model, parent);
		this.type = 'connection-point';
		this.uiState['qualified-type'] = 'vnfd.connection-point';
		this.className = 'ConnectionPoint';
		this.location = 'bottom-left';
		this.position = new Position(parent.position.value());
		this.position.top = parent.position.bottom;
		this.position.width = 14;
		this.position.height = 14;
	}

	get id() {
		return this.model.name;
	}

	get key() {
		return this.parent.key + '/' + this.model.name;
	}

	get name() {
		return this.model.name;
	}

	get vnfdIndex() {
		return this.parent.vnfdIndex;
	}

	get vnfdId() {
		return this.parent.vnfdId;
	}

	get cpNumber() {
		return this.uiState.cpNumber;
	}

	set cpNumber(n) {
		this.uiState.cpNumber = n;
	}

	/**
	 * Convenience method to generate a VnfdConnectionPointRef instance from any given ConnectionPoint.
	 *
	 * @returns {RspConnectionPointRef}
	 */
	toRspConnectionPointRef() {
		const ref = new RspConnectionPointRef({});
		ref.vnfdId = this.vnfdId;
		ref.vnfdIndex = this.vnfdIndex;
		ref.vnfdConnectionPointName = this.name;
		ref.cpNumber = this.cpNumber;
		return ref;
	}

	toVnfdConnectionPointRef() {
		const ref = new VnfdConnectionPointRef({});
		ref.vnfdId = this.vnfdId;
		ref.vnfdIndex = this.vnfdIndex;
		ref.vnfdConnectionPointName = this.name;
		ref.cpNumber = this.cpNumber;
		return ref;
	}

}
