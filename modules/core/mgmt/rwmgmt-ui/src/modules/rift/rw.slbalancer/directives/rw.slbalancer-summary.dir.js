/**
 * Command Center Module
 * @fileoverview Command Center Module parent
 */
(function(window, angular) {
  "use strict";

  angular.module('slbalancer')
    .directive('slbalancerSummary', function() {
      return {
        restrict: 'AE',
        templateUrl: '/modules/views/rw.slbalancer-summary.tmpl.html',
        replace: true,
        controller : function($scope) {
          $scope.$watch('service', function() {
            if (typeof($scope.service) === 'undefined') {
              return;
            }
            // ingress always first???
            // $scope.ingressConnector = $scope.service.connector[0];
            // $scope.egressConnector = $scope.service.connector[1];
          });
        },
        scope : {
          service : '='
        }
      };
    });

})(window, window.angular);
