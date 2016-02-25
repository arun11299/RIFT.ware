module.exports = (function(window, angular){
  "use strict";

  /**
   * <status-led> -
   */
  var controller = function($scope) {
    $scope.isUp = false;
    var updateStateString = function() {
      $scope.stateStringValue = rw.ui.capitalize($scope.stateString ? $scope.stateString : $scope.state);
    }
    $scope.$watch('stateString', updateStateString);
    $scope.$watch('state', function() {
      switch ($scope.state) {
        case 'up':
        case 'OK':
        case 'STARTING':
        case 'STARTED':
        case 'RUNNING':
          $scope.isUp = true;
          break;
        default:
          $scope.isUp = false;
          break;
      }
      updateStateString();
    });
  };

  angular.module('cmdcntr')
    .directive('statusLed', function() {
      return {
        restrict: 'AE',
        template: '<span><div ng-class="{tgStatusIsUp : isUp}" class="led"></div> {{stateStringValue}}</span>',
        controller: controller,
        replace: true,
        scope : {
          state : '@',
          stateString : '@'
        }
      };
    });

})(window, window.angular);
