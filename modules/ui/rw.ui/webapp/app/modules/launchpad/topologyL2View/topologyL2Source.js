
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var API_SERVER = rw.getSearchParams(window.location).api_server;
var NODE_PORT = 3000;
var TopologyL2Actions = require('./topologyL2Actions.js');
var Utils = require('../../utils/utils.js');
import $ from 'jquery';


export default {
  fetchStackedTop() {
    return {
      remote() {
        return new Promise(function (resolve, reject) {
          $.ajax({
                  url: 'http://' + window.location.hostname + ':' + NODE_PORT + '/launchpad/stacked-top',
                  type: 'GET',
                  beforeSend: Utils.addAuthorizationStub,
                  contentType: "application/json",
                  success: function(data) {
                    resolve(data);
                  },
                  error: function(error) {
                    console.log("There was an error getting the stacked top data", error);
                    reject(error);
                  }
              });
        })
      },
      local() {
        return null;
      },
      success: TopologyL2Actions.getTopologyApiSuccess,
      error: TopologyL2Actions.getTopologyApiError,
      loading: TopologyL2Actions.getTopologyApiLoading
    }
  },
  fetchVmTop() {
    return {
      remote() {
        return new Promise(function (resolve, reject) {
          $.ajax({
                  url: 'http://' + window.location.hostname + ':' + NODE_PORT + '/launchpad/stacked-vmtop',
                  type: 'GET',
                  beforeSend: Utils.addAuthorizationStub,
                  contentType: "application/json",
                  success: function(data) {
                    resolve(data);
                  },
                  error: function(error) {
                    console.log("There was an error getting the stacked top data", error);
                    reject(error);
                  }
              });
        })

      },
      local() {
        return null;
      },
      success: TopologyL2Actions.getTopologyApiSuccess,
      error: TopologyL2Actions.getTopologyApiError,
      loading: TopologyL2Actions.getTopologyApiLoading
    }
  }
}
