
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var alt = require('../../core/alt');
function createCloudAccountStore () {
  this.exportAsync(require('./cloudAccountSource.js'));
  this.bindActions(require('./cloudAccountActions.js'));
  this.cloudAccount = {};
  var self = this;
  self.validateErrorMsg = "";
  self.validateErrorEvent = 0;
  this.isLoading = true;
}

createCloudAccountStore.prototype.createLoading = function() {
  this.setState({
    isLoading: true
  });
};
createCloudAccountStore.prototype.createSuccess = function() {
  this.setState({
    isLoading: false
  });
};
createCloudAccountStore.prototype.createFail = function() {
  var xhr = arguments[0];
  this.setState({
    isLoading: false,
    validateErrorEvent: true,
    validateErrorMsg: "There was an error creating your Cloud Account. Please contact your system administrator"
  });
};
createCloudAccountStore.prototype.updateLoading = function() {
  this.setState({
    isLoading: true
  });
};
createCloudAccountStore.prototype.updateSuccess = function() {
  console.log('success', arguments);
  this.setState({
    isLoading: false
  });
};

createCloudAccountStore.prototype.deleteSuccess = function(data) {
  this.setState({
    isLoading: false
  });
  if(data.cb) {
    data.cb();
  }
};

createCloudAccountStore.prototype.getCloudAccountSuccess = function(data) {
  this.setState({
    cloudAccount: data.cloudAccount,
    isLoading:false
  });
};
createCloudAccountStore.prototype.validateError = function(msg) {
  this.setState({
    validateErrorEvent: true,
    validateErrorMsg: msg
  });
};
createCloudAccountStore.prototype.validateReset = function() {
  this.setState({
    validateErrorEvent: false
  });
};



module.exports = alt.createStore(createCloudAccountStore);;

