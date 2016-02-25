/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var alt = require('../../core/alt');
var CloudAccountSource = require('./cloudAccountSource');
var CloudAccountActions = require('./cloudAccountActions.js');

function createCloudAccountStore() {
    this.exportAsync(CloudAccountSource);
    this.bindAction(CloudAccountActions.CREATE_LOADING, this.createLoading);
    this.bindAction(CloudAccountActions.CREATE_SUCCESS, this.createSuccess);
    this.bindAction(CloudAccountActions.CREATE_FAIL, this.createFail);
    this.bindAction(CloudAccountActions.UPDATE_LOADING, this.updateLoading);
    this.bindAction(CloudAccountActions.UPDATE_SUCCESS, this.updateSuccess);
    this.bindAction(CloudAccountActions.DELETE_SUCCESS, this.deleteSuccess);
    this.bindAction(CloudAccountActions.GET_CLOUD_ACCOUNT_SUCCESS, this.getCloudAccountSuccess);
    this.bindAction(CloudAccountActions.GET_CLOUD_ACCOUNTS_SUCCESS, this.getCloudAccountsSuccess);
    this.bindAction(CloudAccountActions.VALIDATE_ERROR, this.validateError);
    this.bindAction(CloudAccountActions.VALIDATE_RESET, this.validateReset);
    this.cloudAccount = {
      name: '',
      'account-type': 'openstack'
    };
    this.cloudAccounts = [];
    this.validateErrorMsg = "";
    this.validateErrorEvent = 0;
    this.isLoading = true;
    this.params = {
        "aws": [{
            label: "Key",
            ref: 'key'
        }, {
            label: "Secret",
            ref: 'secret'
        }],
        "openstack": [{
            label: "Key",
            ref: 'key'
        }, {
            label: "Secret",
            ref: 'secret'
        }, {
            label: "Authentication URL",
            ref: 'auth_url'
        }, {
            label: "Tenant",
            ref: 'tenant'
        }, {
            label: 'Management Network',
            ref: 'mgmt-network'
        }, {
            label: 'Floating IP Pool',
            ref: 'floating-ip-pool',
            optional: true
        }],
        "openmano": [{
            label: "Host",
            ref: 'host'
        }, {
            label: "Port",
            ref: 'port'
        }, {
            label: "Tenant ID",
            ref: 'tenant-id'
        }]
    };
    this.accountType = [{
        "name": "OpenStack",
        "account-type": "openstack",
    }, {
        "name": "Cloudsim",
        "account-type": "cloudsim"
    }, {
        "name": "Open Mano",
        "account-type": "openmano"
    }];
    this.cloud = {
        name: '',
        'account-type': 'openstack',
        params: this.params['openstack']
    };
    this.exportPublicMethods({
      updateCloud: this.updateCloud.bind(this)
    })
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
createCloudAccountStore.prototype.updateCloud = function(cloud) {
    console.log('udpating cloud')
    this.setState({
        cloud: cloud
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
    if (data.cb) {
        data.cb();
    }
};

createCloudAccountStore.prototype.getCloudAccountSuccess = function(data) {
  let cloudAccount = data.cloudAccount;
  let stateObject = {
        cloudAccount: cloudAccount,
        cloud: {
          name: cloudAccount.name,
          'account-type': cloudAccount['account-type'],
          params: cloudAccount[cloudAccount['account-type']]
        },
        isLoading: false
    }
    if(cloudAccount['sdn-account']) {
        stateObject.cloud['sdn-account'] = cloudAccount['sdn-account'];
    }
    this.setState(stateObject);
};
createCloudAccountStore.prototype.getCloudAccountsSuccess = function(data) {
    this.setState({
        cloudAccounts: cloudAccounts || []
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
createCloudAccountStore.prototype.getCloudAccountsSuccess = function(cloudAccounts) {
    console.log('success')
    this.setState({
        cloudAccounts: cloudAccounts || [],
        isLoading: false
    });
};


module.exports = alt.createStore(createCloudAccountStore);;
