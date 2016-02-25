
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var API_SERVER = rw.getSearchParams(window.location).api_server;
var NODE_PORT = 3000;
var TopologyActions = require('./topologyActions.js');
var Utils = require('../../utils/utils.js');
import $ from 'jquery';

export default {
  openNSRTopologySocket: function() {
    return {
      remote: function(state, id) {
        return new Promise(function(resolve, reject) {
          //If socket connection already exists, eat the request.
          if(state.socket) {
            return resolve(false);
          }
           $.ajax({
            url: 'http://' + window.location.hostname + ':' + NODE_PORT + '/socket-polling?api_server=' + API_SERVER ,
            type: 'POST',
            beforeSend: Utils.addAuthorizationStub,
            data: {
              url: API_SERVER + ':' + NODE_PORT + '/launchpad/nsr/' + id + '/topology?api_server=' + API_SERVER
            },
            success: function(data, textStatus, jqXHR) {
              var url = 'ws://' + window.location.hostname + ':' + data.port + data.socketPath;
              var ws = new WebSocket(url);
              resolve(ws);
            }
          }).fail(function(xhr){
            //Authentication and the handling of fail states should be wrapped up into a connection class.
            Utils.checkAuthentication(xhr.status);
          });;
        });
      },
      loading: TopologyActions.openNSRTopologySocketLoading,
      success: TopologyActions.openNSRTopologySocketSuccess,
      error: TopologyActions.openNSRTopologySocketError
    };
  },
  getRawVNFR() {
    return {
      remote: function(state, vnfrID) {
        return new Promise(function(resolve, reject) {
          $.ajax({
            url: 'http://' + window.location.hostname + ':' + NODE_PORT + '/api/operational/vnfr-catalog/vnfr/' + vnfrID + '?api_server=' + API_SERVER,
            type: 'GET',
            beforeSend: Utils.addAuthorizationStub,
            success: function(data) {
              resolve(data);
            }
          });
        })
      },
      loading: TopologyActions.getRawLoading,
      success: TopologyActions.getRawSuccess,
      error: TopologyActions.getRawError
    }
  },
  getRawNSR() {
    return {
      remote: function(state, nsrID) {
        return new Promise(function(resolve, reject) {
          $.ajax({
            url: 'http://' + window.location.hostname + ':' + NODE_PORT + '/api/operational/ns-instance-opdata/nsr/' + nsrID + '?api_server=' + API_SERVER,
            type: 'GET',
            beforeSend: Utils.addAuthorizationStub,
            success: function(data) {
              resolve(data);
            }
          });
        })
      },
      loading: TopologyActions.getRawLoading,
      success: TopologyActions.getRawSuccess,
      error: TopologyActions.getRawError
    }
  },
  getRawVDUR() {
    return {
      remote: function(state, vdurID, vnfrID) {
        return new Promise(function(resolve, reject) {
          $.ajax({
            url: 'http://' + window.location.hostname + ':' + NODE_PORT + '/api/operational/vnfr-catalog/vnfr/' + vnfrID + '/vdur/' + vdurID + '?api_server=' + API_SERVER,
            type: 'GET',
            beforeSend: Utils.addAuthorizationStub,
            success: function(data) {
              resolve(data);
            }
          });
        })
      },
      loading: TopologyActions.getRawLoading,
      success: TopologyActions.getRawSuccess,
      error: TopologyActions.getRawError
    }
  },
}
