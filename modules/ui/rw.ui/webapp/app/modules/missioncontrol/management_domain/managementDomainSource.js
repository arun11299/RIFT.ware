
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var API_SERVER = rw.getSearchParams(window.location).api_server;
var NODE_PORT = 3000;
var managementDomainActions = require('./managementDomainActions.js');
var Utils = require('../../utils/utils.js');
import $ from 'jquery';
var managementDomainSource = {
  /**
   * Creates a new Management Domain
   * @param  {object} state        Reference to parent store state.
   * @param  {object} managementDomain Management Domain payload.
   *                  Should resemble the following:
   *                               {
   *                                 "name": "Management-Domain-One",
   *                                 "vmPool": "VMPool ID",
   *                                 "networkPool": "NetworkPool ID"
   *                               }
   * @return {Promise}
   */
  create: function() {

    return {
      remote: function(state, managementDomain) {
        return new Promise(function(resolve, reject) {
          $.ajax({
            url: 'http://' + window.location.hostname + ':3000/mission-control/mgmt-domain?api_server=' + API_SERVER,
            type:'POST',
            beforeSend: Utils.addAuthorizationStub,
            data: JSON.stringify(managementDomain),
            contentType: "application/json",
            success: function(data) {
              resolve(data);
            },
            error: function(error) {
              console.log("There was an error creating the management domain: ", error);
              reject(error);
            }
          })

        });
      },
      success: managementDomainActions.createSuccess,
      loading: managementDomainActions.createLoading,
      error: managementDomainActions.createFail
    }
  },
  /**
   * Updates a new Management Domain
   * @param  {object} state        Reference to parent store state.
   * @param  {object} managementDomain Management Domain payload.
   *                  Should resemble the following:
   *                               {
   *                                 "name": "Management-Domain-One",
   *                                 "vmPool": "VMPool ID",
   *                                 "networkPool": "NetworkPool ID"
   *                               }
   * @return {Promise}
   */
  update: function() {

    return {
      remote: function(state, managementDomain) {
        return new Promise(function(resolve, reject) {
          $.ajax({
            url: 'http://' + window.location.hostname + ':3000/mission-control/mgmt-domain/' + managementDomain.name + '?api_server=' + API_SERVER,
            type:'PUT',
            beforeSend: Utils.addAuthorizationStub,
            data: JSON.stringify(managementDomain),
            contentType: "application/json",
            success: function(data) {
              resolve(data);
            },
            error: function(error) {
              console.log("There was an error updating the management domain: ", managementDomain.name, error);
              reject(error);
            }
          });
        });
      },
      success: managementDomainActions.updateSuccess,
      loading: managementDomainActions.updateLoading,
      error: managementDomainActions.updateFail
    }
  },
  /**
   * Deletes a new Management Domain
   * @param  {object} state        Reference to parent store state.
   * @param  {object} managementDomain to delete
   * @return {Promise}
   */
  delete: function() {

    return {
      remote: function(state, managementDomain) {
        return new Promise(function(resolve, reject) {
          $.ajax({
            url: 'http://' + window.location.hostname + ':3000/mission-control/mgmt-domain/' + managementDomain + '?api_server=' + API_SERVER,
            type:'DELETE',
            beforeSend: Utils.addAuthorizationStub,
            success: function(data) {
              resolve(data);
            },
            error: function(error) {
              console.log("There was an error deleting the management domain: ", managementDomain, error);
              reject(error);
            }
          })

        });
      },
      success: managementDomainActions.deleteSuccess,
      error: managementDomainActions.deleteFail
    }
  },
  /**
   * Gets pools to be shown in Create/Edit mgmt-domain pages
   *
   * @return {Promise}
   */
  getPools: function() {
    return {
      remote: function() {
        return new Promise(function(resolve, reject) {
          $.ajax({
            url: 'http://' + window.location.hostname + ':3000/mission-control/pool?api_server=' + API_SERVER,
            type: 'GET',
            beforeSend: Utils.addAuthorizationStub,
            success: function(data) {
              resolve(data);
            },
            error: function(error) {
              console.log('There was an error getting pools', error);
              reject(error);
            }
          });
        });
      },
      success: managementDomainActions.getPoolsSuccess,
      error: managementDomainActions.getPoolsFail
    }
  },
  /**
   * Gets management domain to be shown in Edit mgmt-domain page
   *
   * @return {Promise}
   */
  getManagementDomain: function() {
    return {
      remote: function(state, managementDomain) {
        return new Promise(function(resolve, reject) {
          $.ajax({
            url: 'http://' + window.location.hostname + ':3000/mission-control/mgmt-domain/' + managementDomain + '?api_server=' + API_SERVER,
            type: 'GET',
            beforeSend: Utils.addAuthorizationStub,
            success: function(data) {
              resolve({
                managementDomain: data
              });
            },
            error: function(error) {
              console.log('There was an error getting managementDomain', error);
              reject(error);
            }
          });
        });
      },
      success: managementDomainActions.getManagementDomainSuccess,
      error: managementDomainActions.getManagementDomainFail
    }
  }

};
module.exports = managementDomainSource;
