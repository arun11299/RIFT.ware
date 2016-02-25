(function(window, angular) {
  angular.module('trafsim')
    .directive('rwDashboardTrafsimServer', function(vnfFactory, portStateFactory, radio, vcsFactory, trafsimFactory) {
      return {
        restrict: 'AE',
        templateUrl: '/modules/views/rw.trafsim-dashboard_server.tmpl.html',
        replace: true,
        bindToController: true,
        scope: {
          dialDize: '@?',
          panelWidth: '@?',
          trafsim: '='
        },
        controllerAs: 'trafsimDashboardServer',
        controller: controllerFn
      }

      function controllerFn($scope, $rootScope) {
        var self = this;
        var appChannel = radio.channel('appChannel');
        self.appChannel = appChannel;
        self.g = rw.trafsim;
        self.dialSize = this.dialSize || 140;
        self.$scope = $scope;
        self.listeners = [];

        self.gauges = [{
          label: 'Kcalls/s',
          rate: self.gbps,
          max: 210,
          color: 'hsla(212, 57%, 50%, 1)',
          colorLight: 'rgba(55,124,200,0.7)',
          paused: 'false'
        }, {
          label: 'Kmsgs/s',
          rate: self.mpps,
          max: 600,
          color: 'hsla(260, 35%, 50%, 1)',
          colorLight: 'hsla(260, 35%, 50%, 0.7)',
          paused: 'false'
        }];

        trafsimFactory.attached().then(function() {
          self.trafsimClient = trafsimFactory.trafsims[0];
          self.trafsimServer = trafsimFactory.trafsims[1];
        });
        $scope.$watch(function() {
          return rw.trafsim.startedPerceived;
        });

        self.listeners.push(appChannel.on('trafsim-update', function() {

          self.trafsimServer = trafsimFactory.trafsims[1];

          self.gauges[0].rate = self.trafsimServer.call_rate / 1000;
          self.gauges[0].paused = !self.g.startedPerceived;
          self.gauges[1].rate = self.trafsimServer.tx_msg_per_sec / 1000;
          self.gauges[1].paused = !self.g.startedPerceived;

          $scope.$apply();
        }, self));

        self.doCleanup = function() {
          self.listeners = [];
        }

        // extend from BaseController for cleanup
        rw.BaseController.call(this);
      };

      controllerFn.prototype = {

      };

    })
})(window, window.angular);
