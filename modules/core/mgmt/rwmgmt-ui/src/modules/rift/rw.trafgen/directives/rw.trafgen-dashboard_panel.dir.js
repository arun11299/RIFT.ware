(function(window, angular) {
  angular.module('trafgen')
    .directive('rwDashboardTrafgenPanel', function() {
      return {
        restrict: 'AE',
        templateUrl: '/modules/views/rw.trafgen-dashboard_panel.tmpl.html',
        scope: {
        },
        replace:true,
        bindToController: true,
        controllerAs: 'rwDashboardTrafgenPanel',
        controller: function($scope, trafgenFactory) {
          this.l10n = window.rw.ui.l10n;
          trafgenFactory.attached().done(function() {
            console.log("loaded trafgen model");
          });
          $scope.$on("$stateChangeStart", function() {
            trafgenFactory.detached();
          });
        }
      }

    })
})(window, window.angular);
