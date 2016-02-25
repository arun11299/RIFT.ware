(function(window, angular) {
  angular.module('rw.iot')
  .directive('iotArmySummary', function() {
    return {
      restrict: 'E',
      replace: true,
      templateUrl: 'modules/views/rw.iot.army_summary.tmpl.html'
    };
  });
})(window, window.angular);
