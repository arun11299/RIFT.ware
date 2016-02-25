(function(window, angular) {
  angular.module('rw.iot')
  .directive('iotServerPanel', function(vnfFactory, portStateFactory, radio, vcsFactory, iotAppServerFactory) {
    return {
      restrict: 'E',
      templateUrl: '/modules/views/rw.iot.app_server-panel.tmpl.html',
 replace: true,
        bindToController: true,
        scope: {
          dialDize: '@?',
          panelWidth: '@?',
          iotServerName: '@?'
        },
        controllerAs: 'iotDashboardServer',
        controller: controllerFn
      };

      function controllerFn($scope, $rootScope) {
        var self = this;
        var appChannel = radio.channel('appChannel');
        self.appChannel = appChannel;
        self.g = rw.iot;
        self.dialSize = this.dialSize || 140;
        self.$scope = $scope;
        self.listeners = [];

        self.gauges = [{
          label: 'MBPS',
          rate: self.gbps,
          max: 210,
          color: 'hsla(212, 57%, 50%, 1)',
          colorLight: 'rgba(55,124,200,0.7)',
          paused: 'false'
        }, {
          label: 'Msgs/s',
          rate: self.mpps,
          max: 600,
          color: 'hsla(260, 35%, 50%, 1)',
          colorLight: 'hsla(260, 35%, 50%, 0.7)',
          paused: 'false'
        }];

        self.stats = {};

        iotAppServerFactory.attached(self.iotServerName).then(function() {
          console.log('attaching')
          self.iotServer = iotAppServerFactory.data[self.iotServerName];
        });
        $scope.$watch(function() {
          return rw.iot.startedPerceived;
        });

        self.listeners.push(appChannel.on('iot-app-server-update', function() {

          self.iotServer = iotAppServerFactory.data[self.iotServerName];

          self.gauges[0].rate = self.iotServer['receive-rate-bps'] / 1000000;
          self.gauges[0].paused = !self.g.startedPerceived;
          self.gauges[1].rate = self.iotServer['messages-received-rate'];
          self.gauges[1].paused = !self.g.startedPerceived;

          self.stats = self.iotServer;

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
