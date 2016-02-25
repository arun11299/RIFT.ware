(function(window, angular) {
  angular.module('rw.iot')
  .directive('iotServerSummary', function() {
    return {
      restrict: 'E',
      replace: true,
      templateUrl: 'modules/views/rw.iot.app_server_summary.tmpl.html',
      controller: controllerFn,
      controllerAs: 'iotServerSummary',
      scope: {
        service: '=?'
      },
      bindToController: true
    };
  });



 function controllerFn($scope, $rootScope, portStateFactory, radio, iotAppServerFactory, $timeout) {
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

        iotAppServerFactory.attached(self.service).then(function() {
          console.log('attaching', self.service, iotAppServerFactory);
          self.iotServer = iotAppServerFactory.aggregate[self.service.name];
          console.log(self.iotServer)
        });
        $scope.$watch(function() {
          return rw.iot.startedPerceived;
        });

        self.listeners.push(appChannel.on('iot-app-server-update', function() {

          // self.iotServer = iotAppServerFactory.data[self.iotServerName];

          // self.gauges[0].rate = self.iotServer['receive-rate-bps'] / 1000000;
          // self.gauges[0].paused = !self.g.startedPerceived;
          // self.gauges[1].rate = self.iotServer['messages-received-rate'];
          // self.gauges[1].paused = !self.g.startedPerceived;
          console.log('stats update');
          $timeout(function() {
            console.log(self.iotServer, iotAppServerFactory)
            self.stats = iotAppServerFactory.aggregate[self.service.name];
          })


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


})(window, window.angular);
