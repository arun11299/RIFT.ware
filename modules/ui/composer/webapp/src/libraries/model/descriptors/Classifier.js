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
import DescriptorModelFactory from '../DescriptorModelFactory'
import RspConnectionPointRef from './RspConnectionPointRef'
import VnfdConnectionPointRef from './VnfdConnectionPointRef'

/**
 * A ConnectionPoint is always a child of a VNFD. We use it to build VnfdConnectionPointRef instances. So convenience
 * methods are add to access the fields needed to do that.
 */
export default class Classifier extends DescriptorModel {

	static get type() {
		return 'classifier';
	}

	static get className() {
		return 'Classifier';
	}

	static get qualifiedType() {
		return 'nsd.vnffgd.' + Classifier.type;
	}

	constructor(model, parent) {
		super(model, parent);
		this.type = Classifier.type;
		this.uiState['qualified-type'] = Classifier.qualifiedType;
		this.className = Classifier.className;
		this.position = new Position();
	}

	get matchAttributes() {
		if (!this.model['match-attributes']) {
			this.model['match-attributes'] = {};
		}
		return DescriptorModelFactory.newClassifierMatchAttributes(this.model['match-attributes'], this);
	}

	set matchAttributes(obj) {
		this.model['match-attributes'] = obj.model;
	}

	removeMatchAttributes(child) {
		this.matchAttributes = null;
		this.removeChild(child);
	}

	remove() {
		return this.parent.removeClassifier(this);
	}

}
