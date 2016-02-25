(function(window, angular) {
  angular.module('rw.iot')
  .directive('iotArmyDetails', function() {
    return {
      restrict: 'E',
      replace: true,
      templateUrl: 'modules/views/rw.iot.army_details.tmpl.html'
    };
  });
})(window, window.angular);
