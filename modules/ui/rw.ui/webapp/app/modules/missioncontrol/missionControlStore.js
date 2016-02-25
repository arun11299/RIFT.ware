
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var alt = require('../core/alt');
var Utils = require('../utils/utils.js');
import loggingActions from '../logging/loggingActions.js';
import loggingSource from '../logging/loggingSource.js';
import AuthActions from '../login/loginAuthActions.js';
function MissionControlStoreConstructor () {
  var self = this;
  this.exportAsync(require('./missionControlSource.js'));
  this.exportAsync(loggingSource);
  this.bindListeners({
    handleLogout : AuthActions.notAuthenticated

  });
  this.bindActions(require('./missionControlActions.js'));
  this.bindActions(loggingActions);
  this.domains = [];
  this.socket = null;
  this.cloudAccounts = [];
  this.sdnAccounts = [];
  this.validateErrorMsg = "";
  this.validateErrorEvent = 0;
  this.isLoading = false;
  this.exportPublicMethods({
    closeSocket: this.closeSocket.bind(self)
  })
}

MissionControlStoreConstructor.prototype.handleLogout = function(data) {
  this.closeSocket();
};
MissionControlStoreConstructor.prototype.closeSocket = function() {
  if(this.socket) {
    this.socket.close();
  }
  this.setState({
    socket:null
  })
}
MissionControlStoreConstructor.prototype.getSysLogViewerURLError = function(data) {
  if (data.requester == 'mc') {
    this.validateError("Log URL has not been configured.");
  }
};

MissionControlStoreConstructor.prototype.getFederationsSuccess = function(domains) {
  console.log('success')
  this.setState({
    domains: domains || []
  });
};
MissionControlStoreConstructor.prototype.getCloudAccountsSuccess = function(cloudAccounts) {
  console.log('success')
  this.setState({
    cloudAccounts: cloudAccounts || []
  });
};
MissionControlStoreConstructor.prototype.getSdnAccountsSuccess = function(sdnAccounts) {
  console.log('success')
  this.setState({
    sdnAccounts: sdnAccounts || []
  });
}
MissionControlStoreConstructor.prototype.getSysLogViewerURLSuccess = function(data) {
  if (data.requester == 'mc') {
    window.open(data.url);
  }
};
MissionControlStoreConstructor.prototype.openMGMTSocketSuccess = function(connection) {
  var self = this;
  if (!connection) return;
  console.log('connecting socket')
  self.setState({
    socket: connection
  });
  connection.onmessage = function(data) {
    try {
      var data = JSON.parse(data.data);
      if(!data.error) {
        self.setState({
          domains:data || []
        });
      }
      Utils.checkAuthentication(data.statusCode, function() {
        self.closeSocket();
      });
    } catch (e) {

    }
  };
  connection.onerror = function(data) {
    connection.close();
  }
};
MissionControlStoreConstructor.prototype.startLaunchpadSuccess = function() {
  console.log('Launchpad Start Success');
};
MissionControlStoreConstructor.prototype.stopLaunchpadSuccess = function() {
  console.log('Launchpad Stop Success');
};
MissionControlStoreConstructor.prototype.getFederationsFail = function() {
  console.log('getFederationsFail', arguments);
};
MissionControlStoreConstructor.prototype.validateError = function(msg) {
  this.setState({
    validateErrorEvent: true,
    validateErrorMsg: msg
  });
};
MissionControlStoreConstructor.prototype.validateReset = function() {
  this.setState({
    validateErrorEvent: false
  });
};


module.exports = alt.createStore(MissionControlStoreConstructor);;

