(function(window, angular) {
  angular.module('rw.iot.launchpad')
    .factory('rw.iot.launchpad.create', function(radio, $http) {
      var createChannel = radio.channel('createChannel');
      var templatesAPI = '...'
      var createData = {}
      // Returns a list of available services
      createChannel.reply('services', function() {
        return $http.get(templatesAPI + '/services')
      })

      // Returns Pool Data for a service
      createChannel.comply('service:select', function(commandName, id) {
        createData.service = {
          id: id
        };
        $http.get(templatesAPI + '/service/' + id)
          .done(function(data) {
            // Returns pools for selected pool
            createChannel.trigger('service:pools:update', data)
          })
      });

      // Returns Pool Data for a service
      createChannel.comply('pool:select', function(commandName, id) {
        createData.pool = {
          id: id
        };
        $http.get(templatesAPI + '/parameters/' + id)
          .done(function(data) {
            // Returns parameters for selected pool
            createChannel.trigger('service:parameters:update', data)
          })
      });

      createChannel.comply('environment:save', function(commandName) {

      });

      createChannel.comply('environment:launch', function(commandName) {

      })

    });
})(window, window.angular);
