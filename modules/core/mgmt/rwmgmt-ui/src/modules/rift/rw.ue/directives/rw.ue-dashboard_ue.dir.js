(function(window, angular) {

angular.module('ue')
.directive('rwDashboardUe', function() {
  return {
    restrict: 'AE',
    templateUrl: '/modules/views/rw.ue-dashboard_ue.tmpl.html',
    replace: true,
    scope: {
      dialSize:'=?',
      ltesim: '='
    },
    bindToController: true,
    controllerAs: 'dashboardUe',
    controller: function() {

      this.dialSize = this.dialSize || 101;

      this.maxCalls = 100;
    }
  }
});
})(window, window.angular);
