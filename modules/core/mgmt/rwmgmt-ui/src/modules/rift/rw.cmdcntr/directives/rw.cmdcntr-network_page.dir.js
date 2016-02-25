require('../factory/rw.cmdcntr-vcs.factory.js');

module.exports = (function(window, angular) {
  "use strict";

  angular.module('cmdcntr')
  .directive('networkPage', networkPage);

  function networkPage() {

    var controller = function($scope, vnfFactory, radio, $timeout) {
      $scope.services = null;
      var self = this;
      var appChannel = radio.channel('appChannel');
      self.vnfFactory = vnfFactory;

      // The selectInterface ftom network diagram directive will update the iface
      $scope.iface = null;

      self.listeners = [];
      self.listeners.push(appChannel.on('select-interface', function(data) {
        $timeout(function() {
          $scope.iface = data.iface;
        });
      }, self));

      self.vnfFactory.attached().done(function() {

        $timeout(function() {
          $scope.services = vnfFactory.services;

          // Set iface details to none. selectInterface on network diagram
          // will update interface details
          $scope.iface = null;
        });
      });

      $scope.$on('$stateChangeStart', function() {
        self.vnfFactory.detached();
        appChannel.off(null, null, self)
        // self.listeners.forEach(function(listener) {
        //   appChannel.cancel();
        // });

        self.listeners.length = 0;
      });
      rw.BaseController.call(this);
    };

    return {
      restrict: 'AE',
      templateUrl: '/modules/views/rw.cmdcntr-network_page.tmpl.html',
      controller: controller,
      replace: true
    }
  };
})(window, window.angular);
