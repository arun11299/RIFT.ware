
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
'use strict';

import $ from 'jquery'
import alt from '../alt'
import utils from '../libraries/utils'
import CatalogPackageManagerActions from '../actions/CatalogPackageManagerActions'

function getApiServerOrigin() {
	return utils.getSearchParams(window.location).upload_server + ':4567';
}

function ajaxRequest(path, catalogPackage, resolve, reject, method = 'GET') {
	$.ajax({
		url: getApiServerOrigin() + path,
		type: method,
		beforeSend: utils.addAuthorizationStub,
		dataType: 'json',
		success: function(data) {
			if (typeof data == 'string') {
				data = JSON.parse(data);
			}
			resolve({
				data: data,
				state: catalogPackage
			});
		},
		error: function(error) {
			if (typeof error == 'string') {
				error = JSON.parse(error);
			}
			reject({
				data: error,
				state: catalogPackage
			});
		}
	});
}

const CatalogPackageManagerSource = {

	requestCatalogPackageDownload: function () {
		return {
			remote: function (state, download) {
				return new Promise((resolve, reject) => {
					// the server does not add a status in the payload
					// so we add one so that the success handler will
					// be able to follow the flow of this download
					const setStatusBeforeResolve = (response = {}) => {
						response.data.status = 'download-requested';
						resolve(response);
					};
					const path = '/api/export?ids=' + download.ids;
					ajaxRequest(path, download, setStatusBeforeResolve, reject);
				});
			},
			success: CatalogPackageManagerActions.downloadCatalogPackageStatusUpdated,
			error: CatalogPackageManagerActions.downloadCatalogPackageError
		};
	},

	requestCatalogPackageDownloadStatus: function() {
		return {
			remote: function(state, download) {
				const transactionId = download.transactionId;
				return new Promise(function(resolve, reject) {
					const path = '/api/export/' + transactionId + '/state';
					ajaxRequest(path, download, resolve, reject);
				});
			},
			success: CatalogPackageManagerActions.downloadCatalogPackageStatusUpdated,
			error: CatalogPackageManagerActions.downloadCatalogPackageError
		}
	},

	requestCatalogPackageUploadStatus: function () {
		return {
			remote: function (state, upload) {
				const transactionId = upload.transactionId;
				return new Promise(function (resolve, reject) {
					const action = upload.riftAction === 'onboard' ? 'upload' : 'update';
					const path = '/api/' + action + '/' + transactionId + '/state';
					ajaxRequest(path, upload, resolve, reject);
				});
			},
			success: CatalogPackageManagerActions.uploadCatalogPackageStatusUpdated,
			error: CatalogPackageManagerActions.uploadCatalogPackageError
		};
	}

};

export default CatalogPackageManagerSource;
