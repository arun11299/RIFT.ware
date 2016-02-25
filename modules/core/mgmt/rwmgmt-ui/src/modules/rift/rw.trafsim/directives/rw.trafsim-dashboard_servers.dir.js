(function(window, angular) {
  angular.module('trafsim')
  .directive('rwDashboardTrafsimServers', function(vnfFactory, portStateFactory, radio, vcsFactory, trafsimFactory) {
    return {
      restrict: 'AE',
      templateUrl:'/modules/views/rw.trafsim-dashboard_servers.tmpl.html',
      replace: true,
      bindToController: true,
      scope: {
        dialDize: '@?',
        panelWidth: '@?',
        trafsim:'='
      },
      controllerAs: 'trafsimDashboardServers',
      controller: controllerFn
    }
    function controllerFn($scope, radio){
      var self = this;
      var appChannel = radio.channel('appChannel');
      self.appChannel = appChannel;
      self.g = rw.trafsim;
      self.dialSize = this.dialSize || 140;
      $scope.$watch(function() {
        return rw.trafsim.startedPerceived;
      });

      trafsimFactory.attached().then(function(){
        self.trafsimClient = trafsimFactory.trafsims[0];
        self.trafsimServer = trafsimFactory.trafsims[1];
      });
      console.log(self.trafsimClient)
      appChannel.on('trafsim-update', function() {
        self.trafsimServer = trafsimFactory.trafsims[1];
      }, self);
     rw.BaseController.call(this);
    };

    controllerFn.prototype = {

    };

  })
})(window, window.angular);
