/**
 * Command Center Module
 * @fileoverview Command Center Module parent
 */
(function(window, angular) {
  "use strict";
try {
  angular.module('cmdcntr')
} catch(e) {
    angular.module('cmdcntr', ['rwHelpers', 'dispatchesque', 'uiModule', 'ui.router']);
}

  angular.module('cmdcntr')
    .directive('connectorSummary', function() {
      var controller = function($scope, $interval) {
        $scope.theme = rw.theme;
      };
      return {
        restrict: 'AE',
        templateUrl: '/modules/views/rw.cmdcntr-connector_summary.tmpl.html',
        controller : controller,
        replace: true,
        scope : {
          connector : '='
        }
      };
    });


})(window, window.angular);
