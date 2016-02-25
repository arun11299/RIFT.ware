
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var API_SERVER = rw.getSearchParams(window.location).api_server;
var NODE_PORT = 3000;
var poolActions = require('./poolActions.js');
var Utils = require('../../utils/utils.js');
import $ from 'jquery';
var poolSource = {
  /**
   * Creates a new Cloud Account
   * @param  {object} state        Reference to parent store state.
   * @param  {object} cloudAccount Cloud Account payload. Should resemble the following:
   *                               {
   *                                 "name": "Cloud-Account-One",
   *                                 "cloud-type":"mock"
   *                               }
   * @return {[type]}              [description]
   */
  create: function() {
    return {
      remote: function(state, Pool) {
        return new Promise(function(resolve, reject) {
          $.ajax({
            url: 'http://' + window.location.hostname + ':3000/mission-control/pool?api_server=' + API_SERVER,
            type:'POST',
            beforeSend: Utils.addAuthorizationStub,
            data: JSON.stringify(Pool),
            contentType: "application/json",
            success: function(data) {
              resolve(data);
            },
            error: function(error) {
              console.log("There was an error creating the cloud account: ", error);
              reject(error);
            }
          }).fail(function(xhr){
            //Authentication and the handling of fail states should be wrapped up into a connection class.
            Utils.checkAuthentication(xhr.status);
          });

        });
      },
      success: poolActions.createSuccess,
      loading: poolActions.createLoading,
      error: poolActions.createFail
    }
  },
  resources: function() {
    return {
      remote: function(state, cloudAccount) {
        return new Promise(function(resolve, reject) {
          $.ajax({
            url: 'http://' + window.location.hostname + ':3000/mission-control/all-resources?api_server=' + API_SERVER + '&cloud_account=' + cloudAccount,
            type:'GET',
            beforeSend: Utils.addAuthorizationStub,
            contentType: "application/json",
            success: function(data) {
              resolve(data);
            },
            error: function(error) {
              console.log("There was an error retrieving resources by cloud account: ", error);
              reject(error);
            }
          }).fail(function(xhr){
            //Authentication and the handling of fail states should be wrapped up into a connection class.
            Utils.checkAuthentication(xhr.status);
          });
        });
      },
      success: poolActions.getResourcesSuccess,
      error: poolActions.getResourcesFail
    }
  },
  pools: function() {
    return {
      remote: function(state, type, id) {
        return new Promise(function(resolve, reject) {
          var url = 'http://' + window.location.hostname + ':3000/mission-control/pool';
          if (type) {
            url = url + '/' + type;
            if (id) {
              url = url + '/' + id;
            }
          }

          $.ajax({
            url: url + '?api_server=' + API_SERVER,
            type:'GET',
            beforeSend: Utils.addAuthorizationStub,
            dataType: "json",
            contentType: "application/json",
            success: function(data) {
              resolve(data);
            },
            error: function(error) {
              console.log("There was an error getting the pools: ", error);
              reject(error);
            }
          }).fail(function(xhr){
            //Authentication and the handling of fail states should be wrapped up into a connection class.
            Utils.checkAuthentication(xhr.status);
          });
        });
      },
      // What is the difference by getPoolByCloud and pools?
      // success: poolActions.getPoolSuccess,
      success: poolActions.getPoolsSuccess,
      error: poolActions.getPoolsFail
    }
  },
  edit: function() {
    return {
      remote: function(state, Pool) {
        return new Promise(function(resolve, reject) {
          $.ajax({
            url: 'http://' + window.location.hostname + ':3000/mission-control/pool?api_server=' + API_SERVER,
            type:'PUT',
            beforeSend: Utils.addAuthorizationStub,
            data: JSON.stringify(Pool),
            contentType: "application/json",
            success: function(data) {
              resolve(data);
            },
            error: function(error) {
              console.log("There was an error creating the cloud account: ", error);
              reject(error);
            }
          }).fail(function(xhr){
            //Authentication and the handling of fail states should be wrapped up into a connection class.
            Utils.checkAuthentication(xhr.status);
          });

        });
      },
      success: poolActions.editSuccess,
      loading: poolActions.editLoading,
      error: poolActions.editFail
    }
  },
  delete: function() {
    return {
      remote: function(state, pool) {
        return new Promise(function(resolve, reject) {
          $.ajax({
            url: 'http://' + window.location.hostname + ':3000/mission-control/pool?api_server=' + API_SERVER,
            type:'DELETE',
            beforeSend: Utils.addAuthorizationStub,
            dataType: "json",
            data: JSON.stringify(pool),
            contentType: "application/json",
            success: function(data) {
              resolve(data);
            },
            error: function(error) {
              console.log("There was an error creating the cloud account: ", error);
              reject(error);
            }
          }).fail(function(xhr){
            //Authentication and the handling of fail states should be wrapped up into a connection class.
            Utils.checkAuthentication(xhr.status);
          });

        });
      },
      success: poolActions.deleteSuccess,
      error: poolActions.deleteFail
    }
  },
  poolByCloud: function() {
    return {
      remote: function(state, cloudAccount) {
        return new Promise(function(resolve, reject) {
          $.ajax({
            url: 'http://' + window.location.hostname + ':3000/mission-control/get-pools-by-cloud-account?api_server=' + API_SERVER + '&cloud_account=' + cloudAccount,
            type:'GET',
            beforeSend: Utils.addAuthorizationStub,
            dataType: "json",
            contentType: "application/json",
            success: function(data) {
              resolve(data);
            },
            error: function(error) {
              console.log("There was an error creating the cloud account: ", error);
              reject(error);
            }
          }).fail(function(xhr){
            //Authentication and the handling of fail states should be wrapped up into a connection class.
            Utils.checkAuthentication(xhr.status);
          });
        });
      },
      success: poolActions.getPoolByCloudSuccess,
      error: poolActions.getPoolFail
    }
  }
}
module.exports = poolSource;
