
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var ManagementDomainCreateController = function($timeout, $state) {
  var self = this;
  var ManagementDomainStore = require('./managementDomainStore');
  var ManagementDomainActions = require('./managementDomainActions')
  self.ManagementDomainStore = ManagementDomainStore;

  // Get the pools
  self.store = ManagementDomainStore;
  ManagementDomainStore.getPools();

  self.loadingPools = true;

  ManagementDomainActions.validateReset();
  self.openLog = function() {
    console.log('openLog')
    var MissionControlStore = require('../missionControlStore.js');
    MissionControlStore.getSysLogViewerURL('mc');
  }

  self.filterChange = function() {
    var firstPool = true;
    for (var i = 0; i < self.networkPools.length; i++) {
      var pool = self.networkPools[i]
      if (firstPool && !pool.disabled && pool['cloud-account'] === self.cloud_accounts_selector) {
        if (firstPool) {
          self.managementDomain.networkPool = pool.name;
          pool.checked = true;
          firstPool = false
        }
      } else {
        pool.checked = false;
      }
    }
    firstPool = true;
    for (var i = 0; i < self.vmPools.length; i++) {
      var pool = self.vmPools[i]
      if (firstPool && !pool.disabled && pool['cloud-account'] === self.cloud_accounts_selector) {
        if (firstPool) {
          self.managementDomain.vmPool = pool.name;
          pool.checked = true;
          firstPool = false
        }
      } else {
        pool.checked = false;
      }
    }
  }

  self.filterPools = function(type) {
    return function(item) {
      if (self.cloud_accounts_selector === item['cloud-account']) {
        return true
      } else {
        return false;
      }
    }

  }

  self.hasAtLeastOnePool = function(pools) {
    for (var i = 0;pools && i < pools.length; i++) {
      if (pools[i]['cloud-account'] == self.cloud_accounts_selector) {
        return true;
      }
    }
    return false;
  }

  var selector = function(pools, type) {
    if (!pools) {
      return;
    }
    pools.forEach(function(pool, idx) {
      if (pool['mgmt-domain']) {
        // assigned to another domain and unavailable
        pool.disabled = true;
      } else {
        pool.disabled = false;

        //Checks if pool has been selected. If not, then select that pool in the UI and set it in model to be passed to create function.
        if(!self.managementDomain[type + 'Pool']) {
          var firstPool = self[type + 'Pools'][idx];
          firstPool.checked = true;
          self.managementDomain[type + 'Pool'] = firstPool.name;
          //self.managementDomain['networkPool'] = "";
        }
        // Make others unchecked so we only have one checked
        // for (var i = 0; i < self[type + 'Pools'].length; i++) {
        //   if (idx != i) {
        //     self[type + 'Pools'][i].checked = false;
        //   }
        // }
      }
    });
    self.filterChange();
  };

  var listener = function(data) {
    $timeout(function() {
      self.loadingPools = data.loadingPools
      // if (typeof(self.loadingPools) == "undefined") {
      //   self.loadingPools = true;
      // }
      self.vmPools = data.vmPools || [];
      self.networkPools = data.networkPools || [];
      self.portPools = data.portPools || [];
      var all = self.vmPools.concat(self.networkPools, self.portPools);
      self.cloud_accounts = [];
      for (var i = 0; i < all.length; i++) {
        if (all[i] && self.cloud_accounts.indexOf(all[i]['cloud-account']) == -1) {
          self.cloud_accounts.push(all[i]['cloud-account']);
        }
      }
      self.cloud_accounts_selector = self.cloud_accounts[0];
      selector(self.vmPools, 'vm');
      selector(self.networkPools, 'network');
      selector(self.portPools, 'port');
    });
  };

  ManagementDomainStore.listen(listener);
  require('../../utils/utils.js').isNotAuthenticated(window.location, function() {
      ManagementDomainStore.unlisten(listener);
      $state.go('login')
    });
  // To be populated from the UI
  self.managementDomain = {};

  self.create = function() {
    if (typeof(self.managementDomain.name) == "undefined" || self.managementDomain.name == "") {
      ManagementDomainActions.validateError('Please set name for Management Domain');
      return;
    }
    ManagementDomainActions.validateReset();
    console.log('self.managementDomain', self.managementDomain);
     ManagementDomainStore.create(self.managementDomain).then(function() {
      ManagementDomainStore.unlisten(listener);
      console.log(self.managementDomain);
      //$state.go('/');
     });
  };

  self.cancel = function() {
    ManagementDomainStore.unlisten(listener);
    $state.go('/');
  };
};

var ManagementDomainEditController = function($timeout, $state, $stateParams) {
  var self = this;
  var ManagementDomainStore = require('./managementDomainStore');
  var ManagementDomainActions = require('./managementDomainActions')
  self.ManagementDomainStore = ManagementDomainStore;

  self.edit = true;
  self.managementDomain = {
    name: $stateParams.management_domain
  };

  // Get the pools and management domain
  self.store = ManagementDomainStore;
  ManagementDomainStore.getPools();
  // Get the management domain
  ManagementDomainStore.getManagementDomain($stateParams.management_domain);

  self.loadingPools = true;

  ManagementDomainActions.validateReset();
  self.openLog = function() {
    console.log('openLog')
    var MissionControlStore = require('../missionControlStore.js');
    MissionControlStore.getSysLogViewerURL('mc');
  }

  self.filterChange = function() {
    var firstPool = true;
    for (var i = 0; i < self.networkPools.length; i++) {
      var pool = self.networkPools[i]
      if (firstPool && !pool.disabled && pool['cloud-account'] === self.cloud_accounts_selector) {
        if (firstPool) {
          self.managementDomain.networkPool = pool.name;
          pool.checked = true;
          firstPool = false
        }
      } else {
        pool.checked = false;
      }
    }
    firstPool = true;
    for (var i = 0; i < self.vmPools.length; i++) {
      var pool = self.vmPools[i]
      if (firstPool && !pool.disabled && pool['cloud-account'] === self.cloud_accounts_selector) {
        if (firstPool) {
          self.managementDomain.vmPool = pool.name;
          pool.checked = true;
          firstPool = false
        }
      } else {
        pool.checked = false;
      }
    }
  }

  self.filterPools = function(type) {
    return function(item) {
      if (self.cloud_accounts_selector === item['cloud-account']) {
        return true
      } else {
        return false;
      }
    }

  }

  self.hasAtLeastOnePool = function(pools) {
    for (var i = 0;pools && i < pools.length; i++) {
      if (pools[i]['cloud-account'] == self.cloud_accounts_selector) {
        return true;
      }
    }
    return false;
  }

  var selector = function(pools, type) {
    if (!pools) {
      return;
    }
    pools.forEach(function(pool, idx) {
      if (pool['mgmt-domain'] && pool['mgmt-domain'] != self.managementDomain.name) {
        // assigned to another domain and unavailable
        pool.disabled = true;
      } else {
        pool.disabled = false;

        //Checks if pool has been selected. If not, then select that pool in the UI and set it in model to be passed to create function.
        if(!self.managementDomain[type + 'Pool']) {
          var firstPool = self[type + 'Pools'][idx];
          firstPool.checked = true;
          self.managementDomain[type + 'Pool'] = firstPool.name;
        }
      }
    });
    self.filterChange();
  };

  var listener = function(data) {
    $timeout(function() {
      self.loadingPools = data.loadingPools
      self.vmPools = data.vmPools || [];
      self.networkPools = data.networkPools || [];
      self.portPools = data.portPools || [];
      self.managementDomain = data.managementDomain || {};
      var all = self.vmPools.concat(self.networkPools, self.portPools);
      self.cloud_accounts = [];
      for (var i = 0; i < all.length; i++) {
        if (all[i] && self.cloud_accounts.indexOf(all[i]['cloud-account']) == -1) {
          self.cloud_accounts.push(all[i]['cloud-account']);
          if (data.managementDomain.name == all[i]['mgmt-domain']) {
            self.cloud_accounts_selector = all[i]['cloud-account'];
          }
        }
      }
      selector(self.vmPools, 'vm');
      selector(self.networkPools, 'network');
      selector(self.portPools, 'port');
    });
  };


  ManagementDomainStore.listen(listener);
  require('../../utils/utils.js').isNotAuthenticated(window.location, function() {
      ManagementDomainStore.unlisten(listener);
      $state.go('login')
    });

  self.update = function() {
    ManagementDomainActions.validateReset();
    console.log('self.managementDomain', self.managementDomain);
     ManagementDomainStore.update(self.managementDomain).then(function() {
      ManagementDomainStore.unlisten(listener);
      $state.go('/');
     });
  };

  self.cancel = function() {
    ManagementDomainStore.unlisten(listener);
    $state.go('/');
  };
};

var ManagementDomainDeleteController = function($state, $stateParams) {
  var self = this;
  if (confirm("Do you really want to delete management domain " + $stateParams.management_domain + "?")) {
      // FleetStore.deleteFleet(self.fleet.id)
  }
  $state.go('/');
};


module.exports = {
  ManagementDomainCreateController: ManagementDomainCreateController,
  ManagementDomainEditController: ManagementDomainEditController,
  ManagementDomainDeleteController: ManagementDomainDeleteController
};
