angular.module('loadbal')
  .factory('loadbalFactory', ['radio', '$rootScope' , function(radio, $rootScope) {
    var appChannel = radio.channel('appChannel');
    return {

      attached : function(colony) {
        if (this.promise) {
          return this.promise;
        }
        this.workers = [];
        this.sum = {};
        this.socket = new rw.api.SocketSubscriber('web/get');
        var url = '/api/operational/colony/' + colony + '/load-balancer?deep';
        this.socket.websubscribe(url, this.updateData.bind(this), this.offlineRandomize.bind(this));
      },

      updateData: function(config) {
        var nextWorkers = [];
        for (var i = 0; i < config['rw-destnat-data:load-balancer'].fastpath.length; i++) {
          var fpath = config['rw-destnat-data:load-balancer'].fastpath[i];
          for (var j = 0; j < fpath.worker.length; j++) {
            nextWorkers.push(this.workerRow(fpath.worker[j]));
          }
        }

        rw.inplaceUpdate(this.workers, nextWorkers);
        rw.math.run(this.sum, this.workers, rw.math.sum);

        appChannel.on('loadbal-update');
      },

      workerRow: function(wkr) {
        var row = {
          fwdBindings : parseInt(wkr['fwd-bindings']),
          fwdPktReceived :parseInt(wkr['fwd-pkt-received']),
          fwdPktXmit :parseInt(wkr['fwd-pkt-xmit']),
          fwdPktDrop :parseInt(wkr['fwd-pkt-drop']),
          revBindings :parseInt(wkr['rev-bindings']),
          revPktReceived :parseInt(wkr['rev-pkt-received']),
          revPktXmit :parseInt(wkr['rev-pkt-xmit']),
          revPktDrop :parseInt(wkr['rev-pkt-drop'])
        };
        return row;
      },

      offlineRandomize: function(data) {
        console.log('TODO: randomize load balancer');
        this.updateData(data);
      },

      detached: function() {
        this.socket.unsubscribe();
        this.promise = null;
      }
    };
  }]);
