module.exports = angular.module('rw.iot')
  .factory('iotDeviceArmyFactory', ['radio', '$rootScope',
    function(radio, $rootScope) {
      return {
        services: [],
        promise: null,
        attached: function(iotClientName) {
          var self = this;
          self.appChannel = radio.channel('appChannel');

          self.subscriptions = [];

          if (this.promise) {
            return this.promise;
          };

          var deferred = jQuery.Deferred();

          var socket = new rw.api.SocketSubscriber('web/get');
          var url = '/api/operational/colony/' + iotClientName + '/iot-device-army?deep'
          var meta = {
            url: url,
            accept: 'application/vnd.yang.data+json'
          };
          socket.subscribeMeta(self.load.bind(self, iotClientName), meta, self.offline.bind(self, iotClientName));
          self.subscriptions.push(socket);

          self.data = {};

          deferred.resolve();

          self.promise = deferred.promise();
          return self.promise;
        },

        load: function(iotClientName, data) {
          var self = this;

          self.data[iotClientName] = data['rw-iot-data:iot-device-army'].instance[0].service[0].statistics['device-group'][0];

          self.appChannel.trigger('iot-device-army-update');
        },

        offline: function(iotClientName, data) {
          var self = this;

          self.data[iotClientName] = {
            "name": "group1",
            "current-iot-session": 0,
            "total-iot-session-connected": 0,
            "total-iot-session-disconnected": 0,
            "transmit-rate-bps": 2000000,
            "receive-rate-bps": 2000000,
            // "sensor-data-message-received-bps": 0,
            "messages-received-rate": 500,
            "messages-sent-rate": 500,
            // "hdfs-writes-rate": 20,
            "total-message-stanza-sent": 0,
            "total-message-stanza-received": 0,
            "total-query-stanza-sent": 0,
            "total-query-stanza-received": 0,
            "total-sensor-data-message-received": 0,
            // "total-hdfs-writes": 0
          };

          self.appChannel.trigger('iot-device-army-update');
        },

        unsubscribe: function() {
          _.each(this.subscriptions, function(socket) {
            socket.unsubscribe();
          });
          this.subscriptions.length = 0;
        },

        detached: function() {
          this.unsubscribe();
          this.promise = null;
        }
      };
    }
  ]);
