/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
/**
 * Created by onvelocity on 2/8/16.
 */
/*global describe, beforeEach, it, expect, xit, xdescribe */

'use strict';
import _ from 'lodash'
import DescriptorModelSerializer from '../../../src/libraries/model/DescriptorModelSerializer'
import DescriptorModelFactory from '../../../src/libraries/model/DescriptorModelFactory'
import SampleCatalogs from 'json!../../../src/assets/ping-pong-catalog.json'
import TestCatalogs from 'json!../../helpers/test-clean-input-output-model.json'

describe('DescriptorModelFactory', () => {
	it('exists', () => {
		expect(DescriptorModelFactory).toBeDefined();
	});
	describe('buildCatalogItemFactory', () => {
		let containers;
		beforeEach(() => {
			const nsdJson = _.cloneDeep(SampleCatalogs[0].descriptors[0]);
			// the CatalogItemsStore adds the type to the uiState field when the catalog is loaded
			nsdJson.uiState = {type: 'nsd'};
			// the user will open a catalog item by dbl clicking on it in the ui that is when we
			// parse the item into a DescriptorModel class instance as follows....
			const factory = DescriptorModelFactory.buildCatalogItemFactory(SampleCatalogs);
			// the result is a list of all the containers defined with then NSD JSON data
			containers = [nsdJson].reduce(factory, []);
		});
		it('ignores an empty object', () => {
			const factory = DescriptorModelFactory.buildCatalogItemFactory([]);
			const result = [{}].reduce(factory, []);
			expect(result).toEqual([]);
		});
		it('parses an NSD object', () => {
			const nsdJson = _.cloneDeep(SampleCatalogs[0].descriptors[0]);
			nsdJson.uiState = {type: 'nsd'};
			const factory = DescriptorModelFactory.buildCatalogItemFactory(SampleCatalogs);
			const result = [nsdJson].reduce(factory, [])[0];
			expect(result.id).toEqual('ba1dfbcc-626b-11e5-998d-6cb3113b406f');
		});
		it('parses the constituent-vnfd classes', () => {
			const nsd = containers[0];
			const cvnfd = containers.filter(d => DescriptorModelFactory.isConstituentVnfd(d));
			expect(nsd.constituentVnfd).toEqual(cvnfd);
		});
		describe('ConstituentVnfd', () => {
			it('connection-points derive from referenced VNFD', () => {
				const constituentVNFDs = containers.filter(d => DescriptorModelFactory.isConstituentVnfd(d)).map(d => d.vnfdId);
				expect(constituentVNFDs).toEqual(['ba145e82-626b-11e5-998d-6cb3113b406f', 'ba1947da-626b-11e5-998d-6cb3113b406f']);
			});
		});
		describe('DescriptorModelSerializer', () => {
			it('outputs the same JSON that was parsed by the .buildCatalogItemFactory method', () => {
				const inputJSON = _.cloneDeep(TestCatalogs[0].descriptors[0]);
				const inputJSONString = JSON.stringify(inputJSON);
				inputJSON.uiState = {type: 'nsd'};
				const factory = DescriptorModelFactory.buildCatalogItemFactory(TestCatalogs);
				const parsedModel = [inputJSON].reduce(factory, []);
				const serialized = DescriptorModelSerializer.serialize(parsedModel[0].model);
				const result = JSON.stringify(serialized);
				expect(inputJSONString).toEqual(result);
			});
		});
	});
});
