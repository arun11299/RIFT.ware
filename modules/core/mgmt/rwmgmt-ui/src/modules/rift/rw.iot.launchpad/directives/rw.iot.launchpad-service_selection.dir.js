(function(window, angular) {
  angular.module('rw.iot.launchpad')
    .directive('rwIotServiceSelection', function(radio) {
      return {
        restrict: 'AE',
        controllerAs: 'serviceSelection'
        controller: function(radio, $timeout) {
          var self = this;
          var createChannel = radio.channel('createChannel');
          var templateData = createChannel.request('services');
          templateData.done(function(data) {
            $timeout(function() {
              self.services = data;
            })
          });

          self.selectService = function(id) {
            createChannel.command('service:select', id);
          }
        }
      }
    })
})(window, window.angular);
