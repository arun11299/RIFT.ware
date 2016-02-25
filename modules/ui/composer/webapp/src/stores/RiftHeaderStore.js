
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
/**
 * Created by kkashalk on 11/10/15.
 */
'use strict';

import alt from '../alt'
import React from 'react'
import utils from '../libraries/utils'
import RiftHeaderActions from '../actions/RiftHeaderActions'
import CatalogDataSource from '../sources/CatalogDataSource'
import CatalogDataSourceActions from '../actions/CatalogDataSourceActions'

class RiftHeaderStore {

	constructor() {
		this.headerTitle = 'Launchpad: ' + unescape(utils.getSearchParams(window.location).mgmt_domain_name);
		this.nsdCount = 0;
		this.registerAsync(CatalogDataSource);
		this.bindAction(CatalogDataSourceActions.LOAD_CATALOGS_SUCCESS, this.loadCatalogsSuccess);
		this.bindActions(RiftHeaderActions);
	}

	loadCatalogsSuccess(data) {
		let self = this;
		let descriptorCount = 0;
		data.data.forEach((catalog) => {
			descriptorCount += catalog.descriptors.length;
		});

		self.setState({
			descriptorCount: descriptorCount
		});
	}
}

export default alt.createStore(RiftHeaderStore, 'RiftHeaderStore');
