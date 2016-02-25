/**
 * LTE Sim Summary
 */
(function(window, angular) {

  "use strict";

  /**
   * <lte-summary>
   */
  var controller = function($scope, ueFactory) {
    var self = this;
    $scope.theme = rw.theme;
    $scope.$watch(
      function() {
        return self.service;
      },
      function() {
        ueFactory.attached().done(function() {
          self.ltesim = ueFactory.ueForService(self.service);
        });
      }
    );
    $scope.$on('$stateChangeStart', function() {
      ueFactory.detached();
    });
  };

  controller.prototype = {
    maxCallRate : 200000,
    maxMsgRate : 600000
  };

  angular.module('ue')
    .directive('lteSummary', function() {
      return {
        restrict: 'AE',
        templateUrl: '/modules/views/rw.ue-lte_summary.tmpl.html',
        replace: true,
        controller : controller,
        controllerAs: 'lteSummary',
        bindToController: true,
        scope : {
          service : '='
        }
      };
    });

})(window, window.angular);