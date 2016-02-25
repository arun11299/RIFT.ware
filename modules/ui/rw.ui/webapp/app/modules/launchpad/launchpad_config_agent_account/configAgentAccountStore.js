
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var alt = require('../../core/alt');
var ConfigAgentAccountSource = require('./configAgentAccountSource');
var ConfigAgentAccountActions = require('./configAgentAccountActions');

function createconfigAgentAccountStore () {
  this.exportAsync(ConfigAgentAccountSource);
  this.bindAction(ConfigAgentAccountActions.CREATE_SUCCESS, this.createSuccess);
  this.bindAction(ConfigAgentAccountActions.CREATE_LOADING, this.createLoading);
  this.bindAction(ConfigAgentAccountActions.CREATE_FAIL, this.createFail);
  this.bindAction(ConfigAgentAccountActions.UPDATE_SUCCESS, this.updateSuccess);
  this.bindAction(ConfigAgentAccountActions.UPDATE_LOADING, this.updateLoading);
  this.bindAction(ConfigAgentAccountActions.UPDATE_FAIL, this.updateFail);
  this.bindAction(ConfigAgentAccountActions.DELETE_SUCCESS, this.deleteSuccess);
  this.bindAction(ConfigAgentAccountActions.DELETE_FAIL, this.deleteFail);
  this.bindAction(ConfigAgentAccountActions.GET_CONFIG_AGENT_ACCOUNT_SUCCESS, this.getConfigAgentAccountSuccess);
  this.bindAction(ConfigAgentAccountActions.GET_CONFIG_AGENT_ACCOUNT_FAIL, this.getConfigAgentAccountFail);
  this.bindAction(ConfigAgentAccountActions.GET_CONFIG_AGENT_ACCOUNTS_SUCCESS, this.getConfigAgentAccountsSuccess);
  // this.bindAction(ConfigAgentAccountActions.GET_CONFIG_AGENT_ACCOUNTS_FAIL, this.getconfigAgentAccountsFail);
  this.bindAction(ConfigAgentAccountActions.VALIDATE_ERROR, this.validateError);
  this.bindAction(ConfigAgentAccountActions.VALIDATE_RESET, this.validateReset);
  this.exportPublicMethods({
    resetAccount: this.resetAccount.bind(this)
  })
  this.configAgentAccount = {};
  this.configAgentAccounts = [];
  var self = this;
  self.validateErrorMsg = "";
  self.validateErrorEvent = 0;
  this.isLoading = true;
  this.params = {
    "juju": [{
        label: "IP Address",
        ref: 'ip-address'
    }, {
        label: "Port",
        ref: 'port',
        optional: true
    }, {
        label: "Username",
        ref: 'user'
    }, {
        label: "Secret",
        ref: 'secret'
    }]
  };

  this.accountType = [{
      "name": "JUJU",
      "account-type": "juju",
  }];

  this.configAgentAccount = {
      name: '',
      'account-type': 'juju'
  };
  
}

createconfigAgentAccountStore.prototype.resetAccount = function() {
  this.setState({
    configAgentAccount: {
      name: '',
      'account-type': 'juju'
  }
  });
};

createconfigAgentAccountStore.prototype.createLoading = function() {
  this.setState({
    isLoading: true
  });
};
createconfigAgentAccountStore.prototype.createSuccess = function() {
  this.setState({
    isLoading: false
  });
};
createconfigAgentAccountStore.prototype.createFail = function() {
  var xhr = arguments[0];
  this.setState({
    isLoading: false,
    validateErrorEvent: true,
    validateErrorMsg: "There was an error creating your Config Agent Account. Please contact your system administrator"
  });
};
createconfigAgentAccountStore.prototype.updateLoading = function() {
  this.setState({
    isLoading: true
  });
};
createconfigAgentAccountStore.prototype.updateSuccess = function() {
  this.setState({
    isLoading: false
  });
};
createconfigAgentAccountStore.prototype.updateFail = function() {
  var xhr = arguments[0];
  this.setState({
    isLoading: false,
    validateErrorEvent: true,
    validateErrorMsg: "There was an error updating your Config Agent Account. Please contact your system administrator"
  });
};

createconfigAgentAccountStore.prototype.deleteSuccess = function(data) {
  this.setState({
    isLoading: false
  });
  if(data.cb) {
    data.cb();
  }
};

createconfigAgentAccountStore.prototype.deleteFail = function(data) {
  this.setState({
    isLoading: false,
    validateErrorEvent: true,
    validateErrorMsg: "There was an error deleting your Config Agent Account. Please contact your system administrator"
  });
  if(data.cb) {
    data.cb();
  }
};

createconfigAgentAccountStore.prototype.getConfigAgentAccountSuccess = function(data) {

  this.setState({
    configAgentAccount: data.configAgentAccount,
    isLoading:false
  });
};

createconfigAgentAccountStore.prototype.getConfigAgentAccountFail = function(data) {
  this.setState({
    isLoading:false,
    validateErrorEvent: true,
    validateErrorMsg: "There was an error obtaining the data for the Config Agent Account. Please contact your system administrator"
  });
};

createconfigAgentAccountStore.prototype.validateError = function(msg) {
  this.setState({
    validateErrorEvent: true,
    validateErrorMsg: msg
  });
};
createconfigAgentAccountStore.prototype.validateReset = function() {
  this.setState({
    validateErrorEvent: false
  });
};
createconfigAgentAccountStore.prototype.getConfigAgentAccountsSuccess = function(configAgentAccounts) {
  this.setState({
    configAgentAccounts: configAgentAccounts || [],
    isLoading:false
  });
};


module.exports = alt.createStore(createconfigAgentAccountStore);;

