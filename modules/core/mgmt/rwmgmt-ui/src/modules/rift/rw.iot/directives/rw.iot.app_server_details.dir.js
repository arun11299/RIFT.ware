(function(window, angular) {
  angular.module('rw.iot')
  .directive('iotAppServerDetails', function() {
    return {
      restrict: 'E',
      replace: true,
      templateUrl: 'modules/views/rw.iot.app_server_details.tmpl.html'
    };
  });
})(window, window.angular);
