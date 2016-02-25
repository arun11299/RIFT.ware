var moduleDefs = {
  panels: {
    "dashboard": {
      name: "Dashboard",
      icon: 'icon-control-panel'
    },
    "ue-sim": {
      name: "UE-Sim",
      icon: "icon-control-panel"
    },
    "services": {
      name: "Services",
      icon: "icon-servicel"
    },
    "traffic": {
      name: "Traffic",
      icon: "icon-control-panel"
    },
    "interfaces": {
      name: "Interfaces",
      icon: "icon-graph"
    },
    "topology": {
      name: "Topology",
      icon: "icon-control-panel"
    },
    "resources": {
      name: "Resources",
      icon: "icon-cloud-server"
    },
    "configuration": {
      name: "Configuration",
      icon: "icon-control-panel"
    }
  }
};


var webApp = angular.module('integration',['webui','ui.codemirror','ue-sim'])
//
//.config(function($stateProvider, $urlRouterProvider){
//  webApp.urlRouterProvider = $urlRouterProvider;
//  //$urlRouterProvider.otherwise('/ue-sim');
//
//  $stateProvider
//
//    .state('ue-sim',{
//      url:'/ue-sim',
//      templateUrl:'integration/_page/_uesim/uesim.html',
//      controller:'tempUECtrl'
//      ,controllerAs: 'ue'
//
//    })
//
//  .state('dashboard',{
//    url:'/dashboard',
//    templateUrl:'integration/_page/_dashboard/dashboard.html'
//  })
//
//    .state('services',{
//      url:'/services',
//      templateUrl:'integration/_page/_vnf/vnf.html'
//    })
//  .state('interfaces',{
//    url:'/interfaces',
//    templateUrl:'integration/_page/_network/network.html'
//  })
//  .state('traffic',{
//    url:'/traffic',
//    templateUrl:'integration/_page/_traffic/traffic.html'
//  })
//  .state('topology',{
//    url:'/topology',
//    templateUrl:'integration/_page/_topology/topology.html'
//  })
//  .state('configuration',{
//    url:'/configuration',
//    templateUrl:'integration/_page/_config/config.html'
//  })
//
//  .state('resources',{
//    url:'/resources',
//    templateUrl:'integration/_page/_resources/resources.html'
//  })
//
//    //tempUECtrlFn.$inject = ['ueScript']
//    //function tempUECtrlFn (ueScript) {
//    //  var uesim = this;
//    //  uesim.what = 'WHAT';
//    //  ueScript.getList().done(function (data) {
//    //    uesim.scripts = data['test-script'];
//    //    console.log(uesim)
//    //
//    //  });
//    //}
//
//
//})


angular.module('webui',['webui.header','ui.bootstrap','ui.router','vnf','webui.gauge'
  ,'dashboard','network','resources','traffic','topology','config','global', 'ngResource'
,'ue-sim','reactLog', 'uptime'
])

.controller('testcontroller', function($scope, $interval){
  $scope.test_data = 0;
  $interval(function() {
    $scope.test_data += 2;
  }, 2000);
  $scope.testattr = 8;
  $scope.$watch('testattr', function(){
    console.log('testattr ' + $scope.testattr);
  });
})

.controller('globalCtrl',function($scope,globalCharts,$timeout){
  $scope.trafficChart = globalCharts.traffic;
  $scope.ipTraffic = globalCharts.ip_traffic;
  $scope.diameter = globalCharts.diameter;
    $scope.dropCurtain = true;
    $timeout(function(){
      $scope.dropCurtain = false
    },1000)

  $scope.$watch(function(){
    return globalCharts;
  },function(){
    $scope.trafficChart = globalCharts.traffic;
    $scope.ipTraffic = globalCharts.ip_traffic;
    $scope.diameter = globalCharts.diameter;
  },true)
  $scope.toggleCharts = function(chart){console.log('toggling');globalCharts.toggleCharts(chart)};
})