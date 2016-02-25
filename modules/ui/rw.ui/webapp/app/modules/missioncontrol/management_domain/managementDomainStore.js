
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var alt = require('../../core/alt');
function managementDomainStore () {
  this.exportAsync(require('./managementDomainSource.js'));
  this.bindActions(require('./managementDomainActions.js'));
  this.validateErrorMsg = "";
  this.validateErrorEvent = 0;
  this.managementDomain = {};
  this.isLoading = false;
  this.loadingPools = true;
};

managementDomainStore.prototype.createSuccess = function() {
  this.setState({
    isLoading: false
  })
  console.log('success creating mgmt-domain', arguments);
};
managementDomainStore.prototype.createLoading = function() {
  this.setState({
    isLoading: true
  })
};
managementDomainStore.prototype.createFail = function() {
  var xhr = arguments[0];
  this.setState({
    isLoading: false,
    validateErrorEvent: true,
    validateErrorMsg: "There was an error creating your Management Domain. Please contact your system administrator"
  });
};
managementDomainStore.prototype.editLoading = function() {
  this.setState({
    isLoading: true
  })
};

managementDomainStore.prototype.editSuccess = function() {
  console.log('success updating mgmt-domain', arguments);
  this.setState({
    isLoading: false
  })
};

managementDomainStore.prototype.getPoolsSuccess = function(pools) {
	console.log('success getting pools', arguments);
	pools.loadingPools = false;
	this.setState({
    loadingPools: pools.loadingPools,
    networkPools: pools.networkPools,
    vmPools: pools.vmPools
  });
};

managementDomainStore.prototype.getPoolsFail = function() {
	console.log('no getting pools (not ever)');
	this.setState({loadingPools:false})
};

managementDomainStore.prototype.getManagementDomainSuccess = function(managementDomain) {
	console.log('success getting management domain', arguments);
	this.setState(managementDomain);
};

managementDomainStore.prototype.deleteSuccess = function() {
	console.log('success deleting management domain', arguments);
}
managementDomainStore.prototype.validateError = function(msg) {
  this.setState({
    validateErrorEvent: true,
    validateErrorMsg: msg
  });
};
managementDomainStore.prototype.validateReset = function() {
  this.setState({
    validateErrorEvent: false
  });
};

module.exports = alt.createStore(managementDomainStore);

