(function(window, angular) {
angular.module('cmdcntr')
  .factory('navData', function(){
    var f = {};
    var setOrder = function(nav, modules){
      var ordered = 0;
      console.log('sorting', modules)
      return nav.sort(function(a,b){
        return modules.indexOf(a.module) - modules.indexOf(b.module);
      })

    };
    f.get = function(cb){
      var nav = setOrder(window.$rw.nav, angular.module('modules').requires);
      return cb(nav);
    };
    return f;
  })
    .directive('cmdcntrHeader',function(){
        return{
            restrict:'AE',
            templateUrl:'/modules/views/rw.cmdcntr-header.tmpl.html',
            replace:true,
            link: function(scope,element,attrs){

            },
            controller:function($scope,$state,$rootScope,$interval,navData,$location){

                $scope.header_list = [
                  {name:'UE-Sim', icon:'icon-control-panel'},
                  {name:'Services', icon:'icon-service'},
                  {name:'Traffic', icon:'icon-control-panel'},
                  {name:'Interfaces', icon:'icon-graph'},
                  {name:'Topology', icon:'icon-control-panel'},
                  {name:'Resources', icon:'icon-cloud-server'},
                  {name:'Configuration', icon:'icon-html-code'}
                ];
                //$scope.header_list = [
                //];
              var data = navData.get(function(data){
                //window.riftApp.urlRouterProvider.otherwise('/' + data[0].name.toLowerCase());
                $scope.header_list = data;
              });
              $scope.iframe = $location.$$search.iframe;

              $scope.button_on = false;
              $scope.showTest = ($state.current.name == 'dashboard');
                //if ($state.current.name == 'dashboard') {
                //  $scope.showTest = true;
                //} else {
                //  $scope.showTest = false;
                //}
                $scope.toggleTest = function() {
                  rw.aggregateControlPanel.testing = !rw.aggregateControlPanel.testing;
                };

                var avgMs = function(t, n) {
                  return Math.round((t / n) * 1000);
                }

                $rootScope.$on('$stateChangeStart',
                  function() {
                    rw.api.resetStats();
                  }
                );

                $rootScope.$on('$stateChangeSuccess',
                  function() {
                    if ($state.current.name == 'dashboard') {
                      $scope.showTest = true;
                    } else {
                      $scope.showTest = false;
                    }
                  }
                );

                $scope.perf = rw.search_params['perf'];
                $scope.perfClient = false;
                $scope.perfServer = false;
                if ($scope.perf) {
                  var loadPerformance = function(stats) {
                      $scope.stats = stats;
                      $scope.avgNetconfTime = avgMs(stats.netconf.totalTime, stats.netconf.nRequests);
                      $scope.maxNetconfTime = Math.round(stats.netconf.maxTime * 1000);
                      $scope.avgRestTime = avgMs(stats.rest.totalTime, stats.rest.nRequests);
                      $scope.maxRestTime = Math.round(stats.rest.maxTime * 1000);
                      $scope.avgAjaxTime = Math.round(rw.api.tRequestsMs / rw.api.nRequests);
                      $scope.maxAjaxTime = Math.round(rw.api.tRequestMaxMs);
                      var maxByUrl = _.map(stats.rest.maxUrl, function(maxTime, url) {
                        return {url: url, maxTime: maxTime};
                      });
                      maxByUrl = _.first(_.sortBy(maxByUrl, 'maxTime').reverse(), 5);
                      _.each(maxByUrl, function(metric) {
                        metric.maxTime = Math.round(metric.maxTime * 1000);
                      });
                      $scope.maxByUrl = maxByUrl;
                      $scope.$apply();

                      $scope.clientSubscriptions = _.map(stats.clientWebsocket.subscriptions, function(count, url) {
                        return {url: url, count: count};
                      });
                      $scope.serverSubscriptions = _.map(stats.websocket.subscriptions, function(count, url) {
                        return {url: url, count: count};
                      });
                      var ajax = _.map(rw.api.nRequestsByUrl, function(meta, url) {
                        return meta;
                      });
                      $scope.ajax = _.sortBy(ajax, 'max').reverse();
                    };
                  if ($scope.perfSocket === undefined) {
                    $scope.perfSocket = new rw.api.SocketSubscriber('internal/stats');
                    $scope.perfSocket.id = 99999;
                    $scope.perfSocket.subscribeMeta(loadPerformance);
                  }
                }
                $scope.togglePerfClient = function() {
                  $scope.perfClient = ! $scope.perfClient;
                };
                $scope.togglePerfServer = function() {
                  $scope.perfServer = ! $scope.perfServer;
                };
                $scope.perfServerReset = function() {
                  rw.api.put('/internal/stats');
                };
            }
        }
    });
})(window, window.angular);

