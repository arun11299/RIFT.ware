
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var API_SERVER = rw.getSearchParams(window.location).api_server;
var NODE_PORT = 3000;
var MissionControlActions = require('./missionControlActions.js');
var Utils = require('../utils/utils.js');
import $ from 'jquery';
var MissionControlSource = {
  getMgmtDomains: function() {
    return {
      remote: function() {
        return new Promise(function(resolve, reject) {
          $.ajax({
            url: 'http://' + window.location.hostname + ':3000/mission-control/mgmt-domain?api_server=' + API_SERVER,
            type: 'GET',
            beforeSend: Utils.addAuthorizationStub,
            success: function(feds) {
              resolve(feds);
            }
          }).fail(function(xhr){
            //Authentication and the handling of fail states should be wrapped up into a connection class.
            Utils.checkAuthentication(xhr.status);
          });
        });
      },
      success: MissionControlActions.getFederationsSuccess,
      error: MissionControlActions.getFederationsFail
    };
  },
  getCloudAccounts: function() {
    return {
      remote: function() {
        return new Promise(function(resolve, reject) {
          $.ajax({
            url: 'http://' + window.location.hostname + ':3000/mission-control/cloud-account?api_server=' + API_SERVER,
            // url: 'http://' + window.location.hostname + ':3000/mission-control/cloud-account?api_server=' + API_SERVER,
            type: 'GET',
            beforeSend: Utils.addAuthorizationStub,
            success: function(cloudAccounts) {
              resolve(cloudAccounts);
            }
          });
        });
      },
      success: MissionControlActions.getCloudAccountsSuccess,
      error: MissionControlActions.getCloudAccountsFail
    }
  },
  getSdnAccounts: function() {
    return {
      remote: function() {
        return new Promise(function(resolve, reject) {
          $.ajax({
            url: 'http://' + window.location.hostname + ':3000/mission-control/sdn-account?api_server=' + API_SERVER,
            type: 'GET',
            beforeSend: Utils.addAuthorizationStub,
            success: function(sdnAccounts) {
              resolve(sdnAccounts);
            }
          });
        });
      },
      success: MissionControlActions.getSdnAccountsSuccess,
      error: MissionControlActions.getSdnAccountsFail
    }
  },
  openMGMTSocket: function() {
    return {
      remote: function(state) {
        return new Promise(function(resolve, reject) {
          if(state.socket) {
            return resolve(false);
          }
           $.ajax({
            url: 'http://' + window.location.hostname + ':' + NODE_PORT + '/socket-polling?api_server=' + API_SERVER,
            type: 'POST',
            beforeSend: Utils.addAuthorizationStub,
            data: {
              url: API_SERVER + ':' + NODE_PORT + '/mission-control/mgmt-domain?api_server=' + API_SERVER
            },
            success: function(data) {
              var url = 'ws://' + window.location.hostname + ':' + data.port + data.socketPath;
              var ws = new WebSocket(url);
              resolve(ws);
            }
          });
        });
      },
      success: MissionControlActions.openMGMTSocketSuccess,
      error: MissionControlActions.openMGMTError
    };
  },
  startLaunchpad: function() {
    return {
      remote: function(env, name) {
        return new Promise(function(resolve, reject) {
          $.ajax({
            url: 'http://' + window.location.hostname + ':3000/mission-control/mgmt-domain/start-launchpad/' + name + '?api_server=' + API_SERVER,
            type: 'GET',
            beforeSend: Utils.addAuthorizationStub,
            success: function(feds) {
              resolve(feds);
            }
          });
        });
      },
      success: MissionControlActions.startLaunchpadSuccess,
      error: MissionControlActions.startLaunchpadFail
    };
  },
  stopLaunchpad: function() {
    return {
      remote: function(env, name) {
        return new Promise(function(resolve, reject) {
          $.ajax({
            url: 'http://' + window.location.hostname + ':3000/mission-control/mgmt-domain/stop-launchpad/' + name + '?api_server=' + API_SERVER,
            type: 'GET',
            beforeSend: Utils.addAuthorizationStub,
            success: function(feds) {
              resolve(feds);
            }
          });
        });
      },
      success: MissionControlActions.stopLaunchpadSuccess,
      error: MissionControlActions.stopLaunchpadFail
    };
  }
};
module.exports = MissionControlSource;
