/**
 * Integration module
 * @fileoverview UI.1 Integration file.
 * Used to ease the migration from UI1 to UI2
 */



(function(window, angular){
  if(typeof window.riftApp == "undefined"){
  window.riftApp = {
    nav: [],
    createdBy:'integration'
  };
}
  // It is necessary to include the module dependencies here.
  angular.module('integration.uesim',['ui.router'])
    .config(function($stateProvider){
      window.$rw.nav.push({
        module:'integration.uesim',
        name: "UE-SIM",
        icon: "icon-service"
      });
      $stateProvider.state('ue-sim',{
        url:'/ue-sim',
        templateUrl:'integration/_page/_uesim/uesim.html',
        controller:'tempUECtrl'
        ,controllerAs: 'ue'

      })
    });

  angular.module('integration.dashboard',['ui.router'])
    .config(function($stateProvider){

      $stateProvider.state('dashboard',{
        url:'/dashboard',
        templateUrl:'integration/_page/_dashboard/dashboard.html'
      });
    })
    .run(function(){
      window.$rw.nav.push({
        module:'integration.dashboard',
        name: "Dashboard",
        icon: 'icon-control-panel'
      });
    });

  angular.module('integration.services',['ui.router'])
    .config(function($stateProvider){
      window.$rw.nav.push({
        module:'integration.services',
        name: "Services",
        icon: "icon-service"
      });
      $stateProvider.state('services',{
        url:'/services',
        templateUrl:'integration/_page/_vnf/vnf.html'
      });
    });

  angular.module('integration.interfaces',['ui.router'])
    .config(function($stateProvider){
      window.$rw.nav.push({
        module: 'integration.interfaces',
        name: "Interfaces",
        icon: "icon-graph"
      });
      $stateProvider.state('interfaces',{
        url:'/interfaces',
        templateUrl:'integration/_page/_network/network.html'
      });
    });

  angular.module('integration.traffic',['ui.router'])
    .config(function($stateProvider){
      window.$rw.nav.push({
        module: 'integration.traffic',
        name: "Traffic",
        icon: "icon-control-panel"
      });
      $stateProvider.state('traffic',{
        url:'/traffic',
        templateUrl:'integration/_page/_traffic/traffic.html'
      });
    });

  angular.module('integration.topology',['ui.router'])
    .config(function($stateProvider){
      window.$rw.nav.push({
        module:'integration.topology',
        name: "Topology",
        icon: "icon-control-panel"
      });
      $stateProvider.state('topology',{
        url:'/topology',
        templateUrl:'integration/_page/_topology/topology.html'
      });
    });

  angular.module('integration.resources',['ui.router'])
    .config(function($stateProvider){
      window.$rw.nav.push({
        module: 'integration.resources',
        name: "Resources",
        icon: "icon-cloud-server"
      });
      $stateProvider.state('resources',{
        url:'/resources',
        templateUrl:'integration/_page/_resources/resources.html'
      });
    });

  angular.module('integration.configuration',['ui.router'])
    .config(function($stateProvider){
      window.$rw.nav.push({
        module: 'integration.configuration',
        name: "Configuration",
        icon: "icon-control-panel"
      });
      $stateProvider.state('configuration',{
        url:'/configuration',
        templateUrl:'integration/_page/_config/config.html'
      });
    });

})(window, window.angular)
