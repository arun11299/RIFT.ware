/**
 * Command Center Module
 * @fileoverview Command Center Module parent
 */
(function(window, angular) {
  "use strict";

  angular.module('trafsim')
    .directive('trafsimSummary', function() {
      return {
        restrict: 'AE',
        templateUrl: '/modules/views/rw.trafsim_summary.tmpl.html',
        replace: true,
        controller : function($scope, $rootScope, radio, trafsimFactory) {
          var self = this;
          var appChannel = radio.channel('appChannel');
          self.appChannel = appChannel;
          self.trafsimFactory = trafsimFactory;
          $scope.theme = rw.theme;
          $scope.maxCallRate = 210000;
          $scope.maxMsgRate = 600000;
          var listeners = [];
          trafsimFactory.attached().done(function() {
            listeners.push(appChannel.on("trafsim-update", function() {
              if ($scope.service.type === 'trafsimclient') {
                $scope.trafsim = trafsimFactory.findFirstClient();
              } else {
                $scope.trafsim = trafsimFactory.findFirstServer();
              }
            }, self));
          });
          // $scope.$on('$stateChangeStart', function() {
          //   trafsimFactory.detached();
          //   _.each(listeners, function(listener) {
          //     listener.cancel();
          //   });
          // });
           rw.BaseController.call(this);
        },
        scope : {
          service : '='
        }
      };
    });

})(window, window.angular);
