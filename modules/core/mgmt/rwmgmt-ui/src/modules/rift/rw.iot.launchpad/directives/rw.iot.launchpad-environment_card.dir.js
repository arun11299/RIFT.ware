(function(window, angular) {
  angular.module('rw.iot.launchpad')
    .directive('iotLaunchpadEnvironmentCard', function(iotFactory, radio) {
      return {
        restrict: 'E',
        template: '<div>{{mbps}}</div>',
        controller: function($scope, iotFactory, radio) {
          $scope.mbps = 0;
          var appChannel = radio.channel('iotLaunchpad');

          appChannel.on('iot-update', function() {
            $scope.mbps = 1
          }, self);
          appChannel.on('iot-update2', function() {
            $scope.mbps = 2
          }, self);
          iotFactory.attached();

        }
      }
    });
})(window, window.angular);
