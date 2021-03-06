
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
// Require common components
require('../components/components.js');

// Require launchpad dashboard specific files
require('../missioncontrol/cloud_account/cloud-account.css');

var API_SERVER = rw.getSearchParams(window.location).api_server;
var React = require('react');
var LaunchpadApp = require('./launchpad.jsx');
var RecordView = require('./recordViewer/recordView.jsx');
var LaunchNSApp = require('./network_service_launcher/launchNetworkService.jsx');
var AltContainer = require('alt/AltContainer');
var MPFilter = require('./monitoring-params-filter.jsx');
var LaunchpadStore = require('./launchpadFleetStore.js')
var LaunchpadActions = require('./launchpadFleetActions.js')
var Utils = require('../utils/utils.js');
var TopologyView = require('./topologyView/topologyView.jsx');
var TopologyL2View = require('./topologyL2View/topologyL2View.jsx');
var About = require('../about/about.jsx')
var CrashDetails = require('../debug/crash.jsx')
var Crouton = require('react-crouton')
var AccountSidebar = require('./account_sidebar/accountSidebar.jsx');
var LaunchpadCloudAccount = require('./launchpad_cloud_account/launchpadCloudAccount.jsx');
var LaunchpadSdnAccount = require('./launchpad_sdn_account/launchpadSdnAccount.jsx');
var LaunchpadConfigAgentAccount = require('./launchpad_config_agent_account/launchpadConfigAgentAccount.jsx');
angular.module('launchpad', ['ui.router'])
  .config(function($stateProvider, $urlRouterProvider) {
    $urlRouterProvider.otherwise("/launchpad/dashboard");
    $stateProvider.state('/launchpad', {
      url: '/launchpad/:management_domain',
      replace: true,
      template: '<lp-react-dashboard></lp-react-dashboard>',
      controller: function(){},
      controllerAs: 'lp',
      bindToController: true
    });
    $stateProvider.state('/launchpad/ns', {
      url: '/launchpad/:management_domain/launch',
      replace: true,
      template: '<lp-react-launch></lp-react-launch>',
      controller: function(){},
      controllerAs: 'lp',
      bindToController: true
    });
    $stateProvider.state('/launchpad/detail', {
      url: '/launchpad/:management_domain/:lp/detail',
      replace: true,
      template: '<record-view></record-view>',
      controller: function(){},
      controllerAs: 'lp',
      bindToController: true
    });

    $stateProvider.state('/launchpad/topology', {
      url: '/launchpad/:management_domain/:lp/topology',
      replace: true,
      template: '<topology-view></topology-view>',
      controller: function(){},
      controllerAs: 'lp',
      bindToController: true
    });

    $stateProvider.state('/launchpad/topologyL2', {
      url: '/launchpad/:management_domain/:lp/topologyL2',
      replace: true,
      template: '<topology-l2-view></topology-l2-view>',
      controller: function() {},
      controllerAs: 'lp',
      bindToController: true
    });
   // $stateProvider.state('/launchpad/cloud-account', {
   //    url:'/launchpad/:management_domain/cloud-account/dashboard',
   //    replace: true,
   //    templateUrl:'/modules/launchpad/cloud-account-dashboard.html',
   //    controller: require('./cloud-account.js').CloudAccountCreateController,
   //    controllerAs: 'create'
   //  });
   // $stateProvider.state('/launchpad/cloud-account/create', {
   //    url:'/launchpad/:management_domain/cloud-account/create',
   //    replace: true,
   //    templateUrl:'/modules/launchpad/cloud-account.html',
   //    controller: require('./cloud-account.js').CloudAccountCreateController,
   //    controllerAs: 'create'
   //  });
   //  $stateProvider.state('/launchpad/cloud-account/edit', {
   //    url:'/launchpad/:management_domain/cloud-account/:cloud_account/edit',
   //    replace: true,
   //    templateUrl:'/modules/launchpad/cloud-account.html',
   //    controller: require('./cloud-account.js').CloudAccountEditController,
   //    controllerAs: 'create'
   //  });
    $stateProvider.state('/launchpad/cloud-account', {
      url:'/launchpad/:management_domain/cloud-account/dashboard',
      replace: true,
      template:'<launchpad-cloud-account type="dashboard"></launchpad-cloud-account>',
      controller: function(){},
      controllerAs: 'create'
    });
   $stateProvider.state('/launchpad/cloud-account/create', {
      url:'/launchpad/:management_domain/cloud-account/create',
      replace: true,
      template:'<launchpad-cloud-account type="create"></launchpad-cloud-account>',
      controller: function(){},
      controllerAs: 'create'
    });
    $stateProvider.state('/launchpad/cloud-account/edit', {
      url:'/launchpad/:management_domain/cloud-account/:cloud_account/edit',
      replace: true,
      template:'<launchpad-cloud-account type="edit"></launchpad-cloud-account>',
      controller: function(){},
      controllerAs: 'create'
    });
    $stateProvider.state('/launchpad/topologyL2Vm', {
      url: '/launchpad/:management_domain/:lp/topologyL2Vm',
      replace: true,
      template: '<topology-l2-view-vm></topology-l2-view-vm>',
      controller: function() {},
      controllerAs: 'lp',
      bindToController: true
    });
    $stateProvider.state('/launchpad/sdn-account/dashboard', {
      url:'/launchpad/:management_domain/sdn-account/dashboard',
      replace: true,
      template:'<launchpad-sdn-account type="dashboard"></launchpad-sdn-account>',
      controller: function() {},
      controllerAs: 'create'
    });
    $stateProvider.state('/launchpad/sdn-account/create', {
      url:'/launchpad/:management_domain/sdn-account/create',
      replace: true,
      template:'<launchpad-sdn-account type="create"></launchpad-sdn-account>',
      controller: function() {},
      controllerAs: 'create'
    });
    $stateProvider.state('/launchpad/sdn-account/edit', {
      url:'/launchpad/:management_domain/sdn-account/:sdn_account/edit',
      replace: true,
      template:'<launchpad-sdn-account type="edit"></launchpad-sdn-account>',
      controller: function() {},
      controllerAs: 'create'
    });
    $stateProvider.state('/launchpad/config-agent-account', {
      url:'/launchpad/:management_domain/config-agent-account/dashboard',
      replace: true,
      template:'<launchpad-config-agent-account type="dashboard"></launchpad-config-agent-account-account>',
      controller: function(){},
      controllerAs: 'dashboard'
    });
   $stateProvider.state('/launchpad/config-agent-account/create', {
      url:'/launchpad/:management_domain/config-agent-account/create',
      replace: true,
      template:'<launchpad-config-agent-account type="create"></launchpad-config-agent-account>',
      controller: function(){},
      controllerAs: 'create'
    });
    $stateProvider.state('/launchpad/config-agent-account/edit', {
      url:'/launchpad/:management_domain/config-agent-account/:config_agent_account/edit',
      replace: true,
      template:'<launchpad-config-agent-account type="edit"></launchpad-config-agent-account>',
      controller: function(){},
      controllerAs: 'edit'
    });
  })

  // Temporary React wrapper for LP Dashboard
  .directive('lpReactDashboard', function() {
    return {
      restrict: 'AE',
      controller: function($element) {
        function reactRender() {
          React.render(
            React.createElement(LaunchpadApp, null)
            ,
            $element[0]
          );
        }
        reactRender();
      }
    };
  })
  // Temporary React wrapper for LP Launch
  .directive('lpReactLaunch', function() {
    return {
      restrict: 'AE',
      controller: function($element) {
        function reactRender() {
          React.render(
            React.createElement(LaunchNSApp, null)
            ,
            $element[0]
          );
        }
        reactRender();
      }
    };
  })
  .directive('accountSidebar', function() {
    return {
      restrict: 'AE',
      replace: true,
      template: '<div class="accountSidebar"></div>',
      controller: function($element) {
        function reactRender() {
          React.render(
            React.createElement(AccountSidebar, null)
            ,
            $element[0]
          );
        }
        reactRender();
      }
    };
  })
  .directive('launchpadCloudAccount', function() {
    return {
      restrict: 'AE',
      replace: true,
      scope: {
        type: '@'
      },
      template: '<div></div>',
      controller: function($scope, $element) {
        function reactRender() {
          var type = $scope.type;
          React.render(
            React.createElement(LaunchpadCloudAccount, {
              edit: (type == "edit") ? true : false,
              isDashboard: (type == "dashboard") ? true : false
            })
            ,
            $element[0]
          );
        }
        reactRender();
      }
    };
  })
  .directive('launchpadSdnAccount', function() {
    return {
      restrict: 'AE',
      replace: true,
      scope: {
        type: '@'
      },
      template: '<div></div>',
      controller: function($scope, $element) {
        function reactRender() {
          var type = $scope.type;
          React.render(
            React.createElement(LaunchpadSdnAccount, {
              edit: (type == "edit") ? true : false,
              isDashboard: (type == "dashboard") ? true : false
            })
            ,
            $element[0]
          );
        }
        reactRender();
      }
    };
  })
  .directive('launchpadConfigAgentAccount', function() {
    return {
      restrict: 'AE',
      replace: true,
      scope: {
        type: '@'
      },
      template: '<div></div>',
      controller: function($scope, $element) {
        function reactRender() {
          var type = $scope.type;
          React.render(
            React.createElement(LaunchpadConfigAgentAccount, {
              edit: (type == "edit") ? true : false,
              isDashboard: (type == "dashboard") ? true : false
            })
            ,
            $element[0]
          );
        }
        reactRender();
      }
    };
  })
  // Temporary React wrapper for VNFR Page
  .directive('recordView', function() {
    return {
      restrict: 'AE',
      controller: function($element) {
        function reactRender() {
          React.render(
            React.createElement(RecordView, null)
            ,
            $element[0]
          );
        }
        reactRender();
      }
    };
  })

  // Temporary React wrapper for Topology Page
  .directive('topologyView', function() {
    return {
      restrict: 'AE',
      controller: function($element) {
        function reactRender() {
          React.render(
            React.createElement(TopologyView, null)
            ,
            $element[0]
          );
        }
        reactRender();
      }
    };
  })

  // Temporary React wrapper for TopologyL2 Page
  .directive('topologyL2View', function() {
    return {
      restrict: 'AE',
      controller: function($element) {
        function reactRender() {
          console.log("launchpad.js directive on TopologyL2View");
          React.render(
            React.createElement(TopologyL2View, {topologyType: 'single'})
            ,
            $element[0]
          );
        }
        reactRender();
      }
    };
  })

  .directive('topologyL2ViewVm', function() {
    return {
      restrict: 'AE',
      controller: function($element) {
        function reactRender() {
          console.log("launchpad.js directive on TopologyL2ViewVm");
          React.render(
            React.createElement(TopologyL2View, {topologyType: 'vm'})
            ,
            $element[0]
          );
        }
        reactRender();
      }
    };
  })

    // Temporary React wrapper for LP Launch
  .directive('lpReactFilter', function() {
    return {
      restrict: 'AE',
      controller: function($element) {
        LaunchpadStore.listen(function(data) {
          reactRender(data.nsrs);
        })
        function reactRender(nsrs) {
          if (nsrs && nsrs.length > 0) {
            React.render(
                React.createElement(MPFilter, {'nsrs':nsrs})
                ,
                $element[0]
            );
          }
        }
        reactRender();
      }
    };
  })



