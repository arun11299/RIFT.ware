module.exports = angular.module('rw.iot.launchpad')
  .factory('iotFactory', ['radio', '$rootScope',
    function(radio, $rootScope) {
      return {
        services: [],
        promise: null,
        attached: function() {
          var self = this;
          var iotLaunchpad = radio.channel('iotLaunchpad');
          var ws = $websocket.$new('ws://something:12345')
          .$on('$open', function() {
              console.log('tis open');
              iotLaunchpad.trigger('iot-update2')
            })
            .$on('update', function(data) {
              console.log('data');
              console.log(data);
              iotLaunchpad.trigger('iot-update3', data);
            })
          if (this.promise) {
            return this.promise;
          }

          iotLaunchpad.on('startEnv', function(data) {
            $http.post('/someUrl', {msg:'start', env:data.id});
          });

          iotLaunchpad.on('stopEnv', function(data) {
            $http.post('/someUrl', {msg:'stop', env:data.id});
          });

          var deferred = jQuery.Deferred();
          iotLaunchpad.trigger('iot-update');
          deferred.resolve();

          self.promise = deferred.promise();
          return self.promise;
        },
        detached: function() {
        }
      };
    }
  ]);
