module.exports = angular.module('trafgen')

.factory('trafgenFactory', ['radio', '$rootScope', 'portStateFactory', '$timeout', function(radio, $rootScope, portStateFactory, $timeout) {
  var self = this;
  var appChannel = radio.channel('appChannel');
  var firstRun = true;
  return {
    attached: function() {
      if (this.portsPromise) {
        return this.portsPromise;
      }
      var self = this;
      this.created();
      this.portsPromise = portStateFactory.attached();
      this.portsPromise.done(function() {
        self.colonies = portStateFactory.colonies;
        self.coloniesChanged();
      });
      return this.portsPromise;
    },

    created: function() {
      if (firstRun) {
        this.g = rw.trafgen;
        // recognize we've once received actual values from the backend and initialize
        // values to backend values.
        if (this.g.startedActual !== null) {
          this.g.startedPerceived = this.g.startedActual;
        }
        if (this.g.rateActual !== null) {
          this.g.ratePerceived = this.g.rateActual;
        }
        if (this.g.packetSizeActual !== null) {
          this.g.packetSizePerceived = this.g.packetSizeActual;
        }
        this.g.rateDejitterT0 = 0;
        this.rateDejitterTimeout = 60000; //ms

        firstRun = false;
        console.log('first run')
      } else {
        console.log('second run')
      }



      this.subscriptions = [];
      this.info = {};


    },

    coloniesChanged: function() {
      var self = this;
      _.each(this.colonies, function(colony) {
        var socket = new rw.api.SocketSubscriber('web/get');
        var url = '/api/operational/colony/' + colony + '/port-state?select=portname;trafgen-info(*)';
        var meta = {
          url: url,
          accept: 'application/vnd.yang.collection+json'
        };
        socket.subscribeMeta(self.load.bind(self, colony), meta, self.offline.bind(self, colony));
        self.subscriptions.push(socket);
      });
    },

    load: function(colony, data) {
      var self = this;
      this.info[colony] = data.collection['rw-ifmgr-data:port-state'];
      var states = data.collection['rw-ifmgr-data:port-state'];
      var info = {};
      info.byPort = states
        .filter(function(state) {
          return 'rw-trafgen-data:trafgen-info' in state;
        })
        .map(function(state) {
          var info = state['rw-trafgen-data:trafgen-info'];
          info.port = state.portname;
          return info;
        });

      if (info.byPort.length === 0) {
        info = {
          'tx_state': 'Off',
          'tx_rate': 0,
          'pkt_size': 0
        }
      } else if (info.byPort.length > 1) {
        info = this.aggregateMetrics(info.byPort);
      } else {
        _.extend(info, info.byPort[0]);
      }
      this.info[colony] = info;

      // which colony has the trafgen, that determines button state.  see if there
      // is a better way
      if (/trafgen.*/.test(colony)) {
        $timeout(function() {
          self.determineStartedActual(info);
        })

      }
      appChannel.trigger("trafgen-update");
    },

    aggregateMetrics: function(infos) {
      // for values like tx_rate, burst, cycles, the first value is as good as any
      var info = infos[0];

      // aggregate to item 0 with items 1,2,3...
      for (var i = 1; i < infos.length; i++) {
        // Opinion: any are off, they show as all off
        info.tx_state = (infos[i].tx_state == 'Off' ? 'Off' : infos[i].tx_state);
        if ('latency-distribution' in infos[i]) {
          for (var j = 0; j < infos[i]['latency-distribution'].length; j++) {
            info['latency-distribution'][j].packets += infos[i]['latency-distribution'][j].packets;
          }
        }
      }

      info['packet-count'] = infos.reduce(rw.math.sum2('packet-count'), 0);
      info['maximum-latency'] = infos.reduce(rw.math.max('maximum-latency'), 0);
      info['jitter'] = infos.reduce(rw.math.avg2('jitter'), 0);

      return info;
    },

    determineStartedActual: function(info) {
      var started = (info.tx_state === 'On');
      var rate = parseInt(info.tx_rate);
      var packetSize = parseInt(info.pkt_size);
      if (this.g.startedActual === null) {
        this.g.startedPerceived = started;
      }
      if (this.g.rateActual === null) {
        this.g.ratePerceived = rate;
      }
      if (this.g.packetSizeActual === null) {
        this.g.packetSizePerceived = packetSize;
      }
      this.g.startedActual = started;
      var rateDejitterT1 = new Date().getTime();
      console.log((rateDejitterT1 - this.g.rateDejitterT0 > this.rateDejitterTimeout))
      if (rateDejitterT1 - this.g.rateDejitterT0 > this.rateDejitterTimeout) {
        this.g.rateActual = rate;
        this.g.packetSizeActual = packetSize;
      }

    },

    offline: function(colony, data) {
      console.log('trafgen offline data')
      this.g.startedActual = this.g.startedPerceived;
      this.g.rateActual = this.g.ratePerceived;
      this.g.packetSizeActual = this.g.packetSizePerceived;
      this.load(colony, data);
    //   if (!this.ports || this.ports.length == 0) {
    //   return;
    // }

    // if (!('tx_rate_pps' in this.ports[0])) {
    //   this.load(colony, data);
    //   return;
    // }

    // var t = new Date().getTime();
    // var rate = rw.trafgen.ratePerceived;
    // if (rate == 0) {
    //   rate = rw.trafsim.ratePerceived;
    // }
    // var ratePct = (rate / 100);
    // var packetSize = rw.trafgen.packetSize;

    // _.each(this.ports, function(port) {
    //   port.rx_rate_pps = ratePct * 1000000 * port.speed / (8 * packetSize);
    //   port.tx_rate_pps = ratePct * 1000000 * port.speed / (8 * packetSize);
    //   port.rx_rate_mbps = ratePct * port.speed;
    //   port.tx_rate_mbps = ratePct * port.speed;
    //   port.input_errors += rate;
    //   port.output_errors += rate;
    //   port.input_packets += 100 * rate;
    //   port.t = t;
    // });

    // this.aggregateData(t);
    },

    unsubscribe: function() {
      _.each(this.subscriptions, function(socket) {
        socket.unsubscribe();
      });
      this.subscriptions.length = 0;
    },

    detached: function() {
      this.unsubscribe();
      this.portsPromise = null;
    }
  };
}]);
