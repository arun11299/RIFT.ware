
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var angular = require('angular');
var Crouton = require('react-crouton')
var React = require('react')
var ScreenLoader = require('../components/screen-loader/screenLoader.jsx');
var MissionControlDashboard = require('./missionControlDashboard.jsx');
var CloudAccount = require('./cloud_account/cloudAccountWrapper.jsx');
var SdnAccount = require('./sdn_account/sdnAccount.jsx');
var ManagementDomain = require('./management_domain/managementDomain.jsx');
var CrashDetails = require('../debug/crash.jsx')
var About = require('../about/about.jsx')
var Utils = require('../utils/utils.js');
var API_SERVER = rw.getSearchParams(window.location).api_server;
require('./mission-control.css');
require('./cloud_account/cloud-account.css');
require('./management_domain/management-domain.css');
require('./management_domain/management-domain.scss');

require('../styles/common.scss')
angular.module('missioncontrol', ['ui.router'])

.config(function($stateProvider, $urlRouterProvider) {
    // Support currently discontinued for MC
    // $stateProvider.state('/', {
    //   url: '/',
    //   replace: true,
    //   template:'<mission-control-dashboard></mission-control-dashboard>',
    //   controller: 'missionControl',
    //   controllerAs: 'mc',
    //   bindToController: true
    // });
    // $stateProvider.state('cloud-account/create', {
    //   url:'/cloud-account/create',
    //   replace: true,
    //   template:'<cloud-account></cloud-account>',
    //   controller: function() {},
    //   controllerAs: 'create'
    // });
    // $stateProvider.state('cloud-account/editold', {
    //   url:'/cloud-account/:cloud_account/editold',
    //   replace: true,
    //   templateUrl:'/modules/missioncontrol/cloud_account/create-cloud-account.html',
    //   controller: require('./cloud_account/cloud-account.js').CloudAccountEditController,
    //   //controller:'missionControl',
    //   controllerAs: 'create'
    // });
    // $stateProvider.state('cloud-account/edit', {
    //   url:'/cloud-account/:cloud_account/edit',
    //   replace: true,
    //   template:'<cloud-account edit="true"></cloud-account>',
    //   //controller: require('./cloud-account.js').CloudAccountEditController,
    //   controller:'missionControl',
    //   controllerAs: 'create'
    // });

    // $stateProvider.state('sdn-account/create', {
    //   url:'/sdn-account/create',
    //   replace: true,
    //   template:'<sdn-account></sdn-account>',
    //   controller: 'missionControl',
    //   controllerAs: 'create'
    // });
    // $stateProvider.state('sdn-account/edit', {
    //   url:'/sdn-account/edit',
    //   replace: true,
    //   template:'<sdn-account type="edit"></sdn-account>',
    //   controller: 'missionControl',
    //   controllerAs: 'create'
    // });
    // $stateProvider.state('management-domain/createold', {
    //   url: '/management-domain/createold',
    //   replace: true,
    //   templateUrl: '/modules/missioncontrol/management_domain/management-domain-create.html',
    //   controller: require('./management_domain/management-domain.js').ManagementDomainCreateController,
    //   controllerAs: 'create'
    // });
    // $stateProvider.state('management-domain/create', {
    //   url: '/management-domain/create',
    //   replace: true,
    //   template: '<management-domain></management-domain>',
    //   controller: 'missionControl',
    //   controllerAs: 'create'
    // });
    // $stateProvider.state('management-domain/edit', {
    //   url: '/management-domain/:management_domain/edit',
    //   replace: true,
    //   template: '<management-domain edit="true"></management-domain>',
    //   controller: 'missionControl',
    //   controllerAs: 'create'
    // });
    // $stateProvider.state('createPool', {
    //   url: '/pool/:cloud_account/create',
    //   replace: true,
    //   // templateUrl: '/modules/missioncontrol/pool/pool.html',
    //   template: '<pool></pool>',
    //   controller: function(){},
    //   controllerAs: 'create'
    // });
    // $stateProvider.state('pool', {
    //   url: '/pool/:cloud_account/:pool_type/:pool_name',
    //   replace: true,
    //   // templateUrl: '/modules/missioncontrol/pool/pool.html',
    //   template: '<pool></pool>',
    //   controller: function(){},
    //   controllerAs: 'create'
    // });
    $stateProvider.state('debug', {
      url: '/debug',
      replace: true,
      template: '<crash></crash>',
      controller: function(){},
      controllerAs: 'crash'
    });

    $stateProvider.state('about', {
      url: '/about',
      replace: true,
      template: '<about></about>',
      controller: function(){},
      controllerAs: 'about'
    });
    // $urlRouterProvider.otherwise("/");
})

  // Temporary React wrapper for LP Dashboard
.controller('missionControl', function($timeout, $state) {
    var self = this;
    var MissionControlActions = require('./missionControlActions.js');
    var MissionControlStore = require('./missionControlStore.js');
    var MissionControlSource = require('./missionControlSource.js')
    self.MissionControlStore = MissionControlStore;
    var listener = function(data) {
      $timeout(function() {
          self.domains = data.domains;
          self.cloudAccounts = data.cloudAccounts;
          self.sdnAccounts = data.sdnAccounts
      });
    };

    //TODO: Unlisten - Should be when state changes
    // - Create/Edit Cloud Account
    // - Create/Edit Pool
    // - Create/Edit/Delete Management Domain

    MissionControlStore.listen(listener);
    MissionControlStore.openMGMTSocket()
    MissionControlStore.getMgmtDomains();
    MissionControlStore.getCloudAccounts();
    MissionControlStore.getSdnAccounts();
    self.open = function(index) {
        var isOnline = rw.getSearchParams(window.location).api_server;
        if (isOnline) {
            window.open(window.location.origin + '/index.html?api_server=localhost#/launchpad/' + self.federations[index].id)
        } else {
            window.open('#/launchpad/' + self.federations[index].id)
        }

    };

    self.openLog = function() {
      console.log('openLog')
      MissionControlStore.getSysLogViewerURL('mc');
    }

    self.returnActiveFleetCount = function(fed) {
        var count = 0;
        fed.fleets.forEach(function(v) {
            if (v.status == "active") count++;
        });
        return count;
    };
    require('../utils/utils.js').isNotAuthenticated(window.location, function() {
      MissionControlStore.unlisten(listener);
      MissionControlStore.closeSocket();
      $state.go('login');
    });
})
.directive('missionControlDashboard', function() {
  return {
    restrict: 'AE',
    bindToController: true,
    replace:true,
    controllerAs: 'mcd',
    template: '<div></div>',
    controller: function($element, $rootScope) {
      render();
      function render() {
        React.render(React.createElement(MissionControlDashboard, {}), $element[0]);
      }
    }
  }
})
  .directive('cloudAccount', function() {
    return {
      restrict: 'AE',
      template: '<div></div>',
      scope:{
        store:'=',
        edit:'='
      },
      controller: function($element, $scope) {
        render();
        function render() {
          React.render(React.createElement(CloudAccount, {edit:$scope.edit}), $element[0]);
        }
      }
    }
  })

  .directive('sdnAccount', function() {
    return {
      restrict: 'AE',
      template: '<div></div>',
      scope:{
        store:'=',
        type:'@'
      },
      controller: function($element, $scope) {
        render();
        function render() {
          console.log($scope)
          React.render(React.createElement(SdnAccount, {type:$scope.type}), $element[0]);
        }
      }
    }
  })
  .directive('managementDomain', function() {
    return {
      restrict: 'AE',
      template: '<div></div>',
      scope:{
        store:'=',
        edit:'='
      },
      controller: function($element, $scope) {
        render();
        function render() {
          React.render(React.createElement(ManagementDomain, {edit:$scope.edit}), $element[0]);
        }
      }
    }
  })
.directive('crash', function() {
  return {
    restrict: 'AE',
    bindToController: true,
    replace:true,
    controllerAs: 'crash',
    template: '<div></div>',
    controller: function($element) {
      render();
        this.openLog = function() {
          var MissionControlStore = require('./missionControlStore.js');
          console.log('openLog')
          MissionControlStore.getSysLogViewerURL('mc');
        }
        this.loadDashboard = function() {
          window.location = '//' + window.location.hostname + ':8000/index.html?api_server=' + API_SERVER + '#/';
        }
        this.openAbout = function() {
          window.location = '//' + window.location.hostname + ':8000/index.html?api_server=' + API_SERVER + '#/about';
        }
      function render() {
        React.render(React.createElement(CrashDetails, {}), $element[0]);
      }
    }
  }
})
.directive('about', function() {
  return {
    restrict: 'AE',
    bindToController: true,
    replace:true,
    controllerAs: 'about',
    template: '<div></div>',
    controller: function($element) {
        this.openLog = function() {
          var MissionControlStore = require('./missionControlStore.js');
          console.log('openLog')
          MissionControlStore.getSysLogViewerURL('mc');
        }
        this.loadDashboard = function() {
          window.location = '//' + window.location.hostname + ':8000/index.html?api_server=' + API_SERVER + '#/';
        }
        this.openDebug = function() {
          window.location = '//' + window.location.hostname + ':8000/index.html?api_server=' + API_SERVER + '#/crash';
        }
      render();
      function render() {
        React.render(React.createElement(About, {}), $element[0]);
      }
    }
  }
})
.directive('pool', function() {
  return {
    restrict: 'AE',
    bindToController: true,
    replace:true,
    controllerAs: 'pool',
    template: '<div></div>',
    controller: function($element) {
      render();
      function render() {
        React.render(React.createElement(require('./pool/pool.jsx'), {}), $element[0]);
      }
    }
  }
})
.directive('screenLoader', function() {
  return {
    restrict: 'AE',
    scope: {
      store: '='
    },
    bindToController: true,
    controllerAs: 'screenloader',
    controller: function($element) {
      try {
        this.store.listen(function(data) {
        render(data);
      })
      } catch (e) {
        console.log('Check that you\'re passing in a valid store to the screeLoader directive');
      }
      function render(data) {
        React.render(React.createElement(ScreenLoader, {show: data.isLoading}), $element[0]);
      }
    }
  }
})



