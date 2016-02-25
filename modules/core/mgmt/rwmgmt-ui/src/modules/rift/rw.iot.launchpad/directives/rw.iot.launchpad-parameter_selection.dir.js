(function(window, angular) {
  angular.module('rw.iot.launchpad')
    .directive('rwIotPoolSelection', function(radio) {
      return {
        restrict: 'AE',
        controllerAs: 'parameterSelection'
        controller: function(radio, $timeout) {
          var self = this;
          var createChannel = radio.channel('createChannel');

          createChannel.on('service:parameters:update', function(data) {
            $timeout(function() {
              self.parameters = data;
            })
          })


        }
      }
    })
})(window, window.angular);
