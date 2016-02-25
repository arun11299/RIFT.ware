// Place holder for testing directive
(function(window, angular){
  angular.module('ue')
  .factory('ueFactory', function(radio) {
    var appChannel = radio.channel('appChannel');
    return {
      // Initial definitions
      subscriptions: [],
      noRest: false,
      mme: null,
      saegw: null,
      ltesims: [],

      attached: function() {
        var self = this;
        if (self.promise) {
          return self.promise;
        }
        var deferred = jQuery.Deferred();
        var work = rw.api.get('/api/running/colony?select=trafsim-service/name',
              'application/vnd.yang.collection+json');
        work.done(function(data) {
          self.loadConfig(data);
          deferred.resolve();
        });
        this.promise = deferred.promise();
        return self.promise;
      },

      ueForService: function(service) {
        if (service.type === 'ltegwsim') {
          return this.saegw;
        } else if (service.type === 'ltemmesim') {
          return this.mme;
        }
        return null;
      },

      loadConfig: function(data) {
        // if (!this.active) {
        //   return;
        // }
        var self = this;
        self.unsubscribe();
        this.ltesims = [];
        _.each(data.collection['rw-base:colony'], function(colonyRecord) {
          var colonyId = colonyRecord.name;
          _.each(colonyRecord['rw-appmgr:trafsim-service'], function(lteRecord) {
            var ltesim = {
              colonyId : colonyId,
              name : lteRecord.name
            };
            if (lteRecord.name.indexOf('mme') === 0) {
              self.mme = ltesim;
            } else if (lteRecord.name.indexOf('saegw') === 0) {
              self.saegw = ltesim;
            }
            self.ltesims.push(ltesim);
            var url = '/api/operational/colony/' + ltesim.colonyId + '/trafsim-service/' +
                    ltesim.name + '/statistics';
            var socket = new rw.api.SocketSubscriber('web/get');
            socket.websubscribe(url + '/service/counters',
                self.loadMetrics.bind(self, ltesim),
                self.offlineMetrics.bind(self, ltesim));
            self.subscriptions.push(socket);

            if (!self.noRest) {
              var socket = new rw.api.SocketSubscriber('web/get');
              socket.websubscribe(url + '/rest',
                      self.loadRestMetrics.bind(self, ltesim),
                      self.offlineRestMetrics.bind(self, ltesim));
              self.subscriptions.push(socket);
            }
          });
        });
        appChannel.trigger("ue-update-config");
      },


      loadRestMetrics: function(ltesim, metrics) {
        ltesim.restMetrics = metrics['rw-appmgr:rest'];
        appChannel.trigger("ue-update-rest");
      },

      loadMetrics: function(ltesim, metrics) {
        ltesim.metrics = {counters : metrics['rw-appmgr:counters']};
        ltesim.acceptedTxCalls = this.firstValueOf(ltesim.metrics, [
          'counters.mme.transmitted.create-session-request',
          'counters.saegw.transmitted.create-session-response.accepted'
        ]);
        ltesim.acceptedRxCalls = this.firstValueOf(ltesim.metrics, [
          'counters.mme.received.create-session-response.accepted',
          'counters.saegw.received.create-session-request'
        ]);
        appChannel.trigger('ue-update');
      },

      firstValueOf: function(obj, paths) {
        for (var i = 0; i < paths.length; i++) {
          var v = jsonPath.eval(obj, paths[i]);
          if (v.length > 0) {
            return v[0];
          }
        }
        return 0;
      },

      // quasi-random
      offlineMetrics: function(ltesim, metrics) {
        if (!('metrics' in ltesim)) {
          this.loadMetrics(ltesim, metrics);
        }

        // todo
        //this.fire("update");
      },

      offlineRestMetrics: function(ltesim, metrics) {
        if (!('restMetrics' in ltesim)) {
          this.loadRestMetrics(ltesim, metrics);
        }

        // todo
        // this.fire("update-rest");
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

    }
  })
})(window, window.angular);

