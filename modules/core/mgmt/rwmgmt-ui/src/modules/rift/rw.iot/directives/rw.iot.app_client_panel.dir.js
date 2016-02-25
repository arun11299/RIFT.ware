(function(window, angular) {
  angular.module('rw.iot')
  .directive('iotClientPanel', function(vnfFactory, portStateFactory, radio, vcsFactory, iotDeviceArmyFactory) {
    return {
      restrict: 'AE',
      templateUrl: '/modules/views/rw.iot.app_client-panel.tmpl.html',
      replace: true,
      bindToController: true,
      scope: {
        dialDize: '@?',
        panelWidth: '@?',
        iotClientName:'@?'
      },
      controllerAs: 'iotDashboardClient',
      controller: controllerFn
    }
    function controllerFn($scope, $rootScope){
      var self = this;
      var appChannel = radio.channel('appChannel');
      self.appChannel = appChannel;
      self.g = rw.iot;
      self.dialSize = this.dialSize || 140;

      // Needs to be on this object as BaseController walks and cleans up
      // on this object
      self.$scope = $scope;
      self.listeners = [];

      self.gauges = [
        {label: 'MBPS', rate: self.gbps, max: 210, color: 'hsla(212, 57%, 50%, 1)', colorLight: 'rgba(55,124,200,0.7)', paused: 'false'},
        {label: 'Msgs/s', rate: self.mpps, max: 600, color: 'hsla(260, 35%, 50%, 1)', colorLight: 'hsla(260, 35%, 50%, 0.7)', paused: 'false'}
      ];

      self.stats = {};

      $scope.$watch(function() {
        return rw.iot.startedPerceived;
      });

      iotDeviceArmyFactory.attached(self.iotClientName).then(function(){
        self.iotClient = iotDeviceArmyFactory.data[self.iotClientName];
      });

      self.listeners.push(appChannel.on('iot-device-army-update', function() {
        self.iotClient = iotDeviceArmyFactory.data[self.iotClientName];

        self.gauges[0].rate = self.iotClient['transmit-rate'] / 1000000;
        self.gauges[0].paused = !self.g.startedPerceived;
        self.gauges[1].rate = self.iotClient['messages-sent-rate'];
        self.gauges[1].paused = !self.g.startedPerceived;

        self.stats = self.iotClient;

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
