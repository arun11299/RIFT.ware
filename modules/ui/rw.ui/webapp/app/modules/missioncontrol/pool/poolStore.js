
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var alt = require('../../core/alt');
var poolSource = require('./poolSource.js');
var poolActions = require('./poolActions.js');
function poolStore () {
  let hashParams = window.location.hash.split('/');
  let accountName = hashParams[2];
  let poolType = hashParams[3] || "";
  let poolName = hashParams[4] || "";
   // Create/Edit Setup
  if (poolType == "create") {
    this.isCreatePage = true;
    poolType = 'vm'
  } else {
    this.isCreatePage = false;
  }
  this.isLoading = true;
  this.poolTypes = ["vm", "network", "port"];
  this.pool = {
    "cloud-account": accountName,
    "dynamic-scaling": true,
    name: poolName,
    type: poolType,
  };
  this.availableResources = [];
  this.selectedResources = [];
  this.resources = {
    vm:[],
    network:[]
  };
  this.validateErrorEvent = 0;
  this.validateErrorMsg = "";

  this.exportAsync(poolSource);
  this.bindActions(poolActions);
  this.exportPublicMethods({
    addSelectedResources: this.addSelectedResources.bind(this),
    addAllResources: this.addAllResources.bind(this),
    removeSelectedResources: this.removeSelectedResources.bind(this),
    removeAllResources: this.removeAllResources.bind(this),
    requestPage: this.requestPage.bind(this),
    updateName: this.updateName.bind(this),
    updatePoolType: this.updatePoolType.bind(this),
    updateDynamicScaling: this.updateDynamicScaling.bind(this)
  })
}
poolStore.prototype.requestPage = function() {
  let hashParams = window.location.hash.split('/');
  let accountName = hashParams[2];
  let poolType = hashParams[3] || "";
  let poolName = hashParams[4] || "";
  let isCreatePage;
   // Create/Edit Setup
  if (poolType == "create") {
    isCreatePage = true;
    poolType = 'vm'
  } else {
    isCreatePage = false;
  }
  let pool = {
    "cloud-account": accountName,
    "dynamic-scaling": true,
    name: poolName,
    type: poolType,
  };
  let availableResources = [];
  let selectedResources = [];
  let resources = {
    vm:[],
    network:[]
  };
  this.setState({
    isCreatePage: isCreatePage,
    pool: pool,
    availableResource: availableResources,
    selectedResources: selectedResources,
    resources: resources

  })
}
poolStore.prototype.updateDynamicScaling = function(isDynamic) {
  let pool = this.pool;
  pool["dynamic-scaling"] = isDynamic;
  this.setState({
    pool:pool
  });
};
poolStore.prototype.updatePoolType = function(type) {
  let pool = this.pool;
  pool.type = type;
  this.setState({
    pool:pool
  });
};
poolStore.prototype.addAllResources = function() {
  let poolType = this.pool.type;
  let resources = this.resources;
  let resourceType = resources[poolType];
  for (var i = 0; i < resourceType.length; i++) {
    var node = resourceType[i];
    node.selected = true;
    node.checked = false;
  }
  this.setState({
    resources: resources
  });
};
poolStore.prototype.addSelectedResources = function(data) {
  let poolType = this.pool.type;
  let resources = data;
  let resourceType = resources[poolType];
  for (var i = 0; i < resourceType.length; i++) {
    var node = resourceType[i];
    if (node.checked && !node.selected) {
      node.checked = false;
      node.selected = true;
    }
  }
  this.setState({
    resources: resources
  });
};
poolStore.prototype.removeAllResources = function() {
  let poolType = this.pool.type;
  let resources = this.resources;
  let resourceType = resources[poolType];
  for (var i = 0; i < resourceType.length; i++) {
    var node = resourceType[i];
    node.selected = false;
    node.checked = false;
  }
  this.setState({
    resources: resources
  });
};
poolStore.prototype.removeSelectedResources = function(data) {
  let poolType = this.pool.type;
  let resources = data;
  let resourceType = resources[poolType];
  for (var i = 0; i < resourceType.length; i++) {
    var node = resourceType[i];
    if (node.checked && node.selected) {
      node.checked = false;
      node.selected = false;
    }
  }
  this.setState({
    resources: resources
  });
};
poolStore.prototype.createSuccess = function() {
  this.setState({
    isLoading: false
  });
  window.location.hash = '#/';
  console.log('success', arguments);
};
poolStore.prototype.createLoading = function() {
  this.setState({
    isLoading: true
  });
  console.log('success', arguments);
};
poolStore.prototype.editLoading = function() {
  this.setState({
    isLoading: true
  });
  console.log('success', arguments);
};
poolStore.prototype.editSuccess = function() {
  console.log('success', arguments);
  this.setState({
    isLoading: false
  });
  window.location.hash = '#/';
};
poolStore.prototype.deleteSuccess = function() {
  console.log('success', arguments);
  window.location.hash = '#/';
};
poolStore.prototype.getResourcesSuccess = function(resources) {
  var decRes = resources;
  var poolTypes = this.poolTypes;
  poolTypes.forEach(function(type) {
    if (decRes[type]) {
      decRes[type].forEach(function(item, index) {
        if(item.available == "false") {
          item.available = false;
        } else {
          item.available = true;
        }
        item.selected = false;
      })
    }
  })
  this.setState({resources:decRes,
    isLoading:false});
};
poolStore.prototype.getPoolByCloudSuccess = function(data) {
  var self = this;
  var pools = data;
  var poolTypes = this.poolTypes;
  var poolData = this.pool;
  var resourceData = [];
  if (pools) {
    //This logic should be in the node server. Need to refactor the API.
    // Availability of resources should be indicated in the resource object as well to match what is returned by the all resources api request.
    poolTypes.forEach(function(type, i) {
      var poolTypeExists = pools[type + "Pools"];
      if(!poolTypeExists) return;
      pools[type + "Pools"].forEach(function(pool, j) {
        if (pool.name == self.pool.name) {
          var resourceAvailability = ["assigned", "available"];
          poolData['dynamic-scaling'] = pool['dynamic-scaling'];
          poolData.type = type;
          poolData.typeDisplay = type;
          resourceAvailability.forEach(function(ra, k) {
            if (pool[ra]) {
              pool[ra].forEach(function(r, l) {
                var isAvailable = (ra == "available");
                r.available = isAvailable;
                resourceData.push(r);
              })
            }
          });
          // Add selected state
          resourceData.forEach(function(d, k) {
            d.selected = false;
          })
        }
      })
    })
  }
  console.log('success', resourceData)
  this.setState({pool:poolData,
    resources: resourceData,
    isLoading:false})
};
poolStore.prototype.getPoolsSuccess = function(data) {
  var resourceData = {};
  var poolType = this.pool.type;
  resourceData[poolType] = [];
  var resources = resourceData[poolType];
  data[0].available.map(function(r) {
    r.available = true;
    r.selected = false;
    resources.push(r);
  });
  if (data[0].assigned) {
    data[0].assigned.map(function(r) {
      r.available = true;
      r.selected = true;
      resources.push(r);
    });
  }
  this.setState({resources:resourceData,
    isLoading:false})
};
poolStore.prototype.getResourcesFail = function(e) {
  console.log(e);
}
poolStore.prototype.updateName = function(name) {
  var pool = this.pool;
  console.log(name)
  pool['name'] = name;
 this.setState({
  pool: pool
 })
}
poolStore.prototype.validateError = function(message) {
  this.setState({
    validateErrorMsg: message,
    validateErrorEvent: true
  });
}
poolStore.prototype.validateReset = function() {
  this.setState({
    validateErrorEvent: false
  });
}
module.exports = alt.createStore(poolStore);;

