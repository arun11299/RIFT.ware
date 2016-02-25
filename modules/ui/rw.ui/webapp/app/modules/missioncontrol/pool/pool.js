
// /*
//  * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
//  */
// var poolStore = require('./poolStore.js')
// var poolActions = require('./poolActions.js')
// var PoolController = function($timeout, $state, $stateParams) {
//   var self = this;
//   self.poolStore = poolStore;
//   if ($stateParams.pool_name == "create" || $stateParams.pool_name == "") {
//     self.isCreate = true;
//     self.type = "create";
//     self.pool = {
//       name: "",
//       "cloud-account": $stateParams.cloud_account,
//       type: "vm"
//     };
//   } else {
//     self.isCreate = false;
//     self.type = "edit";
//     self.pool = {
//       name: $stateParams.pool_name,
//       "cloud-account": $stateParams.cloud_account,
//       type:"",
//       typeDisplay:""
//     }
//     poolStore.pools($stateParams.cloud_account);
//     require('../../utils/utils.js').isNotAuthenticated(window.location, function() {
//       $state.go('login')
//     });
//     //poolStore.poolByCloud($stateParams.cloud_account);
//   }


//   self.pool['dynamic-scaling'] = true;

//   self.store = poolStore;
//   poolStore.resources($stateParams.cloud_account)
//   poolStore.listen(function(data) {
//     data.vmResources = data.networkResources = data.portResources = [];
//     $timeout(function() {
//         console.log('updating', data)
//         // If an Edit page
//         if (!self.isCreate) {
//           //If the data returned has a pools and doesn't have a type.
//           if (data.pools && self.pool.type == "") {
//             // For each of the pools
//             for (var prop in data.pools) {
//               if (data.pools.hasOwnProperty(prop)) {
//                 // for each resource in the pools
//                 for (var i = 0; i < data.pools[prop].length; i++) {
//                   // if the resource is named
//                   if (self.pool.name == data.pools[prop][i].name) {
//                     var propType = prop.substring(0, prop.length - 5);
//                     self.pool['dynamic-scaling'] = data.pools[prop][i]['dynamic-scaling'];
//                     self.pool.type = propType;
//                     self.pool.typeDisplay = propType;
//                     self[prop + "Resources"] = self[propType + "Resources"] = [];
//                     // if the resource is available and we can view the resources data
//                     if (data.pools[prop][i].available && data.pools[prop][i].available.length > 0 && data.resources) {
//                       data.pools[prop][i].available.forEach(function(el) {
//                         data.resources[propType].forEach(function(named_resource) {
//                           if (el.id == named_resource.id) {
//                             el.name = named_resource.name;
//                           }
//                         })
//                         el.selected = false;
//                         el.available = true;
//                         self[prop + "Resources"].push(el);
//                       })
//                       // To reset the process so that data.resources are assigned.
//                     } else if (!data.resources) {
//                       self.pool.type = ""
//                     }
//                     if (data.pools[prop][i].assigned && data.pools[prop][i].assigned.length > 0) {
//                       data.pools[prop][i].assigned.forEach(function(el) {
//                         data.resources[propType].forEach(function(named_resource) {
//                           if (el.id == named_resource.id) {
//                             el.name = named_resource.name;
//                           }
//                         })
//                         el.selected = true;
//                         el.available = true;
//                         self[prop + "Resources"].push(el);
//                       })
//                     }
//                   }
//                 }
//               }
//             }
//           }
//         } else {
//           if (data.resources.vm && !self.vmResources) {
//             data.resources.vm.forEach(function(el, index) {el.selected = false});
//             self.vmResources = data.resources.vm
//           }
//           if (data.resources.network && !self.networkResources) {
//             data.resources.network.forEach(function(el, index) {el.selected = false});
//             self.networkResources = data.resources.network;
//           }
//           if (data.resources.port) {
//             data.resources.port.forEach(function(el, index) {el.selected = false});
//             self.portResources = data.resources.port;
//           }
//         }
//     });
// });
//   self.openLog = function() {
//     console.log('openLog')
//     var MissionControlStore = require('../missionControlStore.js');
//     MissionControlStore.getSysLogViewerURL('mc');
//   }
//   self.create = function() {
//     if (self.pool.name == "") {
//       poolActions.validateError("Please give the pool a name");
//       return;
//     }
//     poolActions.validateReset();
//     var resources = [];
//     switch (self.pool.type) {
//       case 'vm':

//         resources = self.vmResources;
//         break;
//       case 'network':
//         resources = self.networkResources;
//         break;
//       case 'port':
//         resources = self.portResources;
//         break;
//       default:
//         console.log('why are you here?');
//     }
//     var assign = [];
//     if (resources) {
//         resources.forEach(function(v) {
//           if(v.selected && v.available == "true") {
//             assign.push({id:v.id});
//           }
//         })
//       }
//     self.pool.assigned = assign;
//      poolStore.create(self.pool).then(function() {
//       $state.go('/');
//       return;
//      });
//   };
//   // self.clickCheckAll = function(type) {
//   //     for (var i = 0; i < self[type + "Resources"].length; i++) {
//   //       var node = self[type + "Resources"][i];
//   //       node.selected = self.checkAll[type];
//   //     }
//   // };
//   // self.checkAllBoxes = function(event, obj, type) {
//   //   obj.selected = !obj.selected;
//   //   if (type == self.pool.type) {
//   //     for (var i = 0; i < self[type + "Resources"].length; i++) {
//   //       var node = self[type + "Resources"][i];
//   //       if (!node.selected) {
//   //         self.checkAll[type] = false;
//   //         return;
//   //       }
//   //     }
//   //     console.log('all')
//   //     self.checkAll[type] = true;

//   //   }
//   // };
//   self.removeAll = function(type) {
//     console.log('fdskladjskl;')
//     for (var i = 0; i < self[type + "Resources"].length; i++) {
//       var node = self[type + "Resources"][i];
//       node.selected = false;
//     }
//   }
//   self.addAll = function(type) {
//     for (var i = 0; i < self[type + "Resources"].length; i++) {
//       var node = self[type + "Resources"][i];
//       if (node.available) {
//         node.selected = true;
//       }
//     }
//   }
//   self.addSelected = function(type) {
//     for (var i = 0; i < self[type + "Resources"].length; i++) {
//       var node = self[type + "Resources"][i];
//       if (node.checked && !node.selected) {
//         node.checked = false;
//         node.selected = true;
//       }
//     }
//   }
//   self.removeSelected = function(type) {
//     for (var i = 0; i < self[type + "Resources"].length; i++) {
//       var node = self[type + "Resources"][i];
//       if (node.checked && node.selected) {
//         node.checked = false;
//         node.selected = false;
//       }
//     }
//   }
//   self.availableVMResources = function() {
//     for (var i = 0; self.vmResources && i < self.vmResources.length; i++) {
//       if (self.vmResources[i].available) {
//         return true;
//       }
//     }
//     return false;
//   }

//   self.availableNetworkResources = function() {
//     for (var i = 0; self.networkResources && i < self.networkResources.length; i++) {
//       if (self.networkResources[i].available) {
//         return true;
//       }
//     }
//     return false;
//   }
//   self.cancel = function() {
//     $state.go('/');
//   }
//   self.delete = function() {
//     console.log(self.pool)
//      poolStore.delete(self.pool).then(function() {
//       $state.go('/');
//      });
//   }
//   self.edit = function() {
//     console.log(self.pool);
//     var resources = [];
//     switch (self.pool.type) {
//       case 'vm':
//         resources = self.vmResources;
//         break;
//       case 'network':
//         resources = self.networkResources;
//         break;
//       case 'port':
//         resources = self.portResources;
//         break;
//       default:
//         console.log('why are you here?');
//     }
//     var assign = [];
//     resources.forEach(function(v) {
//       if(v.selected) {
//         assign.push({id:v.id});
//       }
//     })
//     self.pool.assigned = assign;
//      poolStore.edit(self.pool).then(function() {
//       $state.go('/');
//      });
//   }

// };

// module.exports = PoolController;
