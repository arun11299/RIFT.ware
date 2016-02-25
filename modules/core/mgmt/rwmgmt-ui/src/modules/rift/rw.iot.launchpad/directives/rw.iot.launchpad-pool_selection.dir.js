(function(window, angular) {
  angular.module('rw.iot.launchpad')
    .directive('rwIotPoolSelection', function(radio) {
      return {
        restrict: 'AE',
        controllerAs: 'poolSelection'
        controller: function(radio, $timeout) {
          var self = this;
          var createChannel = radio.channel('createChannel');

          createChannel.on('service:pools:update', function(data) {
            $timeout(function() {
              self.pools = data;
            })
          })

          self.selectPool = function(id) {
            createChannel.command('pool:select', id);
          }
        }
      }
    })
})(window, window.angular);
