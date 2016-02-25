angular.module('header.data', ['ngResource'])

  .provider('navDatas',function(){
    var trafsimtrafgen = [
      {name:'Dashboard', icon:'icon-control-panel'},
      {name:'Services', icon:'icon-service'},
      {name:'Traffic', icon:'icon-control-panel'},
      {name:'Interfaces', icon:'icon-graph'},
      {name:'Topology', icon:'icon-control-panel'},
      {name:'Resources', icon:'icon-cloud-server'},
      {name:'Configuration', icon:'icon-html-code'}
    ];
    var uesim = [
      {name:'UE-Sim', icon:'icon-control-panel'},
      //{name:'Services', icon:'icon-service'},
      {name:'Traffic', icon:'icon-control-panel'},
      //{name:'Interfaces', icon:'icon-graph'},
      {name:'Topology', icon:'icon-control-panel'},
      //{name:'Resources', icon:'icon-cloud-server'},
      //{name:'Configuration', icon:'icon-html-code'}
    ];

    var uesimsanssim = [
      //{name:'UE-Sim', icon:'icon-control-panel'},
      //{name:'Services', icon:'icon-service'},
      {name:'Traffic', icon:'icon-control-panel'},
      //{name:'Interfaces', icon:'icon-graph'},
      {name:'Topology', icon:'icon-control-panel'},
      //{name:'Resources', icon:'icon-cloud-server'},
      //{name:'Configuration', icon:'icon-html-code'}
    ]

    this.$get = headerNavFn;


      //headerNavFn.$inject = ['headerData','$q','$state'];
    function headerNavFn (headerData,$q,$state,$location) {
      return {
        getNav: function(){
          return $q(function(resolve,reject){
            headerData.get({}, function(json) {
              var config = json.config;
              ;
              var data;
              var isDefault = $location.path() === '' || $location.path() === '/';
              var data = json.config.indexOf('UE-Sim') > -1 ? uesim : trafsimtrafgen;
              if(json.config.indexOf('UE-Sim') > -1 || json.config[0].indexOf('ltesim') > -1){
                if(json.config[0] == 'ltesim_saegw'){
                  data = uesimsanssim;
                  webApp.urlRouterProvider.otherwise('/traffic');
                  if(isDefault){
                    $state.go('traffic')
                  }
                  console.log($location.path())
                }else{
                  data = uesim;
                  webApp.urlRouterProvider.otherwise('/ue-sim');
                  if($location.path() === ''){
                    $state.go('ue-sim')
                  }
                }

              } else {

                data = trafsimtrafgen;
                webApp.urlRouterProvider.otherwise('/dashboard');
                if($location.path() === ''){
                  $state.go('dashboard')
                }
              }
              resolve(data)
            }, function() {
              webApp.urlRouterProvider.otherwise('/dashboard');
              resolve(trafsimtrafgen)
            });
          })
        }
      }
    }
  })


  .factory('headerData', function ($resource) {
    return $resource('launchpad/config');
  })
  .directive('alertThis', function(){
    return{
      restrict: 'CAE',
      link: function (scope, element, attr) {
        alert('alert!!')
      }
    }
  })