(function(window, angular) {
angular.module('uiModule')
  .directive('rwGaugeset', function() {
  return {
    restrict: 'AE',
    scope: {
      dialSize: '@',
      panelWidth: '@',
      gauges: '='
    },
    bindToController: true,
      replace: true,
      controllerAs:'gaugeSet',

    controller: function ($scope) {
      var self = this;
      this.dialSize = this.dialSize || 280;
      this.panelWidth = this.panelWidth || 250;
    },
    templateUrl: '/modules/views/rw.ui-gaugeset.tmpl.html'
  }
})
})(window, window.angular);
