module.exports = angular.module('rw.iot')
  .factory('iotAppServerFactory', ['radio', '$rootScope',
    function(radio, $rootScope) {
      return {
        services: [],
        promise: null,
        aggregate: {},
        attached: function(iotServerName) {
          var self = this;
          self.appChannel = radio.channel('appChannel');
          self.subscriptions = [];

          if (this.promise) {
            return this.promise;
          };

          var deferred = jQuery.Deferred();

          var socket = new rw.api.SocketSubscriber('web/get');
          var url = '/api/operational/colony/' + iotServerName.name + '/iot-application-server?deep'
          var meta = {
            url: url,
            accept: 'application/vnd.yang.data+json'
          };
          socket.subscribeMeta(self.load.bind(self, iotServerName.name), meta, self.offline.bind(self, iotServerName.name));
          self.subscriptions.push(socket);

          self.data = {};
          deferred.resolve();

          self.promise = deferred.promise();
          return self.promise;
        },

        load: function(iotServerName, data) {
          var self = this;
          console.log("loading")
          self.data[iotServerName] = data['rw-iot-data:iot-application-server'].instance[0].service[0].statistics['device-group'][0];
          // console.log('appServer', iotServerName, data);
          var iot = data['rw-iot-data:iot-application-server'].instance;
          self.aggregate[iotServerName] = {};
          iot.forEach(function(instance) {
            instance.service.forEach(function(service){
              service.statistics["device-group"].forEach(function(device) {
                for(k in device) {
                if(k != "name") {
                  if(self.aggregate[iotServerName].hasOwnProperty(k)) {
                    self.aggregate[iotServerName][k] += device[k];
                  } else {
                    self.aggregate[iotServerName][k] = device[k];
                  }
                }
              }
              });
            });
          });
          self.appChannel.trigger('iot-app-server-update');
        },

        offline: function(iotServerName, data) {
          var self = this;

          self.data[iotServerName] = {
            "name": "group1",
            "current-iot-session": 0,
            "total-iot-session-connected": 0,
            "total-iot-session-disconnected": 0,
            "transmit-rate-bps": 2000000,
            "receive-rate-bps": 2000000,
            "sensor-data-message-received-bps": 0,
            "messages-received-rate": 500,
            "messages-sent-rate": 500,
            "hdfs-writes-rate": 20,
            "total-message-stanza-sent": 0,
            "total-message-stanza-received": 0,
            "total-query-stanza-sent": 0,
            "total-query-stanza-received": 0,
            "total-sensor-data-message-received": 0,
            "total-hdfs-writes": 0
          };

          self.appChannel.trigger('iot-app-server-update');
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
