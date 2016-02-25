/**
 * Command Center Module
 * @fileoverview Command Center Module parent
 */
module.exports = (function(window, angular) {
  "use strict";

  angular.module('trafgen')
    .directive('trafgenSummary', function($compile, trafgenFactory) {
      return {
        restrict: 'AE',
        templateUrl: '/modules/views/rw.trafgen-summary.tmpl.html',
        replace: true,
        controller : function($scope) {
          trafgenFactory.attached();

          $scope.$on('$stateChangeStart', function() {
            trafgenFactory.detached();
          });

          // assumes trafgens/trafsinks have single connector
          $scope.$watch('service', function() {
            if (typeof($scope.service) === 'undefined') {
              return;
            }
            $scope.connector = $scope.service.connector[0];
          });
        },
        scope : {
          service : '='
        }
      };
    });

})(window, window.angular);
