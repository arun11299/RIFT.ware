
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var SdnAccountCreateController = function($timeout, $state) {
  var self = this;
  var SdnAccountStore = require('./createSdnAccountStore')
  var SdnAccountActions = require('./createSdnAccountActions')
  self.SdnAccountStore = SdnAccountStore;
  self.accountType = [
    {
      "name":"ODL",
      "account-type":"odl",
    },
    {
      "name":"Static",
      "account-type":"static",
    }
  ];
  self.params = {
    "odl": [
        {
          label: "Username",
          ref: 'username'
        },
        {
          label: "Password",
          ref: 'Password'
        },
        {
          label: "URL",
          ref: 'url'
        }
      ],
      "static": [

        ]
  }
  self.sdn = {
    name: '',
    'account-type': 'odl'
  };
  self.store = SdnAccountStore;
  self.openLog = function() {
    console.log('openLog')
    var MissionControlStore = require('./missionControlStore.js');
    MissionControlStore.getSysLogViewerURL('mc');
  }
  self.create = function() {
    if (self.sdn.name == "") {
      SdnAccountActions.validateError('Please give the cloud account a name');
      return;
    } else {
      var type = self.sdn['account-type'];
      if (typeof(self.params[type]) != 'undefined') {
        var params = self.params[type];
        for (var i = 0; i < params.length; i++) {
          var param = params[i].ref;
          if (typeof(self.sdn[type]) == 'undefined' || typeof(self.sdn[type][param]) == 'undefined' || self.sdn[type][param] == "") {

            SdnAccountActions.validateError("Please fill all account details");
            return;
          }
        }
      }
    }
    SdnAccountActions.validateReset();

    //This is here to remove properties not properly dealt with on the backend.
    self.temp_sdn = {name:self.sdn.name}
    console.log(self.temp_sdn);
    SdnAccountStore.create(self.temp_sdn).then(function() {
      $state.go('/');
    });
  }
  self.cancel = function() {
    $state.go('/');
  }
  require('../../utils/utils.js').isNotAuthenticated(window.location, function() {
    $state.go('login')
  });
};

var SdnAccountEditController = function($timeout, $state, $stateParams) {
  var self = this;
  var SdnAccountStore = require('./createSdnAccountStore')
  self.SdnAccountStore = SdnAccountStore;

  // Mark this is edit
  self.edit = true;

  self.accountType = [
    {
      "name":"SDN",
      "account-type":"sdn",
    }
  ];
  self.params = {
    "sdn": [
        {
          label: "Username",
          ref: 'username'
        },
        {
          label: "Password",
          ref: 'Password'
        },
        {
          label: "Key",
          ref: 'key'
        }
      ]
  }
  self.sdn = {
    name: '',
    'account-type': 'sdn'
  };

  // Setup listener
  var listener = function(data) {
    $timeout(function() {
      console.log('updating', data);
      self.sdn.name = data.sdnAccount.name;
      var accountType = self.sdn['account-type'] = data.sdnAccount['account-type'];
      if (data.sdnAccount[accountType]) {
        // There are extra params.
        // Initialize object
        self.sdn[accountType] = {};
        // Iterate over them and assign to proper keys
        for (var k in data.sdnAccount[accountType]) {
          self.sdn[accountType][k] = data.sdnAccount[accountType][k];
        }
      }
    });
  };

  SdnAccountStore.listen(listener);
  require('../../utils/utils.js').isNotAuthenticated(window.location, function() {
    SdnAccountStore.unlisten(listener);
    $state.go('login')
  });
  // Get the cloud account
  SdnAccountStore.getSdnAccount($stateParams.sdn_account);
  self.openLog = function() {
    console.log('openLog')
    var MissionControlStore = require('./missionControlStore.js');
    MissionControlStore.getSysLogViewerURL('mc');
  }
  self.update = function() {
     SdnAccountStore.update(self.sdn).then(function() {
      SdnAccountStore.unlisten(listener);
      $state.go('/');
     });
  };
  self.delete = function() {
    SdnAccountStore.delete(self.sdn.name).then(function() {
      SdnAccountStore.unlisten(listener);
      $state.go('/');
    });
  };
  self.cancel = function() {
    SdnAccountStore.unlisten(listener);
    $state.go('/');
  }
};

module.exports = {
  SdnAccountCreateController: SdnAccountCreateController,
  SdnAccountEditController: SdnAccountEditController
};
