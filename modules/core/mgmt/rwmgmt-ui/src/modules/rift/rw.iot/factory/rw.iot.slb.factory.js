
angular.module('rw.iot')
  .factory('iotSlbFactory', function(radio, $rootScope, portStateFactory) {
  var appChannel = radio.channel('appChannel');

    return {

      created: function () {
        this.metricsByFpath = {};
        this.dnsByFpath = {};
        this.radiusByFpath = {};
        this.sockets = [];
        // in a given 2s, this fires 2N update events, two for each colony.  We consolidate
        // events by debouncing them and causing few update cycles.
        this.debouncedFireUpdateEvent = _.debounce(this.fireUpdateEvent, 1000);
      },

      attached: function () {
        if (this.promise) {
          return this.promise;
        }
        this.created();
        var self = this;
        var deferred = jQuery.Deferred();
        portStateFactory.attached().done(function () {
          _.each(portStateFactory.colonies, function (colony) {
            var url = '/api/operational/colony/' + colony + '/network-context?deep';
            var work = rw.api.get(url, 'application/vnd.yang.collection+json');
            work.done(function(contexts) {
              self.loadConfig(colony, contexts);
              if (typeof(self.slb) != 'undefined') {
                deferred.resolve();
                return false;
              }
            });
          });
        });
        this.promise = deferred.promise();
        return this.promise;
      },

      loadConfig: function (colony, contexts) {
        var self = this;
        var slbs = jsonPath.eval(contexts, 'collection.rw-base:network-context[*].rw-fpath:scriptable-lb[*]');
        if (typeof(slbs) == 'undefined' || slbs.length === 0) {
          return;
        }

        // system should only have one
        this.slb = slbs[0];
        this.unsubscribe();
        // We found the colony that has the stats.
        // Unfortunately we have to subscribe to each fpath stats separately.  Maybe this is
        // good use case for server-side aggregation.
        var url = '/api/operational/colony/' + colony + '/scriptable-lb/fastpath';
        rw.api.get(url, 'application/vnd.yang.collection+json').done(function (fpaths) {
          _.each(fpaths.collection['rw-fpath-data:fastpath'], function (fpath) {
            var socket = new rw.api.SocketSubscriber('web/get');
            var statsUrl = url + '/' + fpath.instance + '/service/' + colony;
            socket.websubscribe(statsUrl,

              self.loadMetrics.bind(self, colony),
              self.offlineMetrics.bind(self, fpath.instance));
            self.sockets.push(socket);

            // var protoSocket = new rw.api.SocketSubscriber('web/get');
            // var protoStatsUrl = url + '/' + fpath.instance + '/service/abc/proto-specific-stats';
            // protoSocket.websubscribe(protoStatsUrl,
            //   self.loadProtoMetrics.bind(self, fpath.instance),
            //   self.offlineProtoMetrics.bind(self, fpath.instance));
            // self.sockets.push(protoSocket);
          });
        });
      },

      loadMetrics: function (instanceId, data) {
        var self = this;
        this.metricsByFpath[instanceId] = this.metricsByFpath[instanceId] || {};
        var stats = data['rw-fpath-data:service'].statistics;
        this.metricsByFpath[instanceId] = stats;
        this.slb.metrics = this.slb.metrics || {};
        this.aggregate(this.slb.metrics, _.values(this.metricsByFpath));

        this.debouncedFireUpdateEvent();
      },

      loadProtoMetrics: function () {
        // var dnsMetrics = jsonPath.eval(data.fastpath, '*.proto-specific-stat.dns-lb-stat.qname-stat[*]');
        // if (dnsMetrics && dnsMetrics.length >= 0) {
        //   this.dnsByFpath[instanceId] = {};
        //   this.mapOccurances(this.dnsByFpath[instanceId], dnsMetrics, 'query-name');
        //   this.slb.dnsMetrics = {};
        //   this.aggregateOccurances(this.slb.dnsMetrics, _.values(this.dnsByFpath));
        // }

        // var radiusMetrics = jsonPath.eval(data.fastpath, '*.proto-specific-stat.radius-lb-stat.username-stat[*]');
        // if (radiusMetrics && radiusMetrics.length >= 0) {
        //   this.radiusByFpath[instanceId] = {};
        //   this.mapOccurances(this.radiusByFpath[instanceId], radiusMetrics, 'domain-name');
        //   this.slb.radiusMetrics = {};
        //   this.aggregateOccurances(this.slb.radiusMetrics, _.values(this.radiusByFpath));
        // }

        this.debouncedFireUpdateEvent();
      },

      fireUpdateEvent: function () {
        // console.log('metricsByFpath', this.metricsByFpath)
        appChannel.trigger("iotSlb-update");
      },

      mapOccurances: function (obj, domains, key) {
        _.each(domains, function (domain) {
          obj[domain[key]] = domain.occurance;
        });
      },

      aggregateOccurances: function (obj, fpathDomains) {
        _.each(fpathDomains, function (domains) {
          _.map(domains, function (occurances, domain) {
            if (domain in obj) {
              obj[domain] += occurances;
            } else {
              obj[domain] = occurances;
            }
          });
        });
      },

      aggregate: function (obj, children) {
        var keys = [
          "active-fwd-session",
          "active-rev-session",
          "clnt-fwd-pkt-rcvd",
          "srvr-fwd-pkt-sent",
          "srvr-rev-pkt-rcvd",
          "clnt-rev-pkt-sent",
          "clnt-fwd-byte-rcvd",
          "srvr-fwd-byte-sent",
          "srvr-rev-byte-rcvd",
          "clnt-rev-byte-sent",
          "fwd-pkt-drop",
          "fwd-pkt-drop-no-session",
          "fwd-pkt-drop-max-session",
          "rev-pkt-drop",
          "rev-pkt-drop-no-session",
          "clnt-fwd-rate-pps",
          "srvr-fwd-rate-pps",
          "srvr-rev-rate-pps",
          "clnt-rev-rate-pps",
          "clnt-fwd-rate-mbps",
          "srvr-fwd-rate-mbps",
          "srvr-rev-rate-mbps",
          "clnt-rev-rate-mbps"
        ];

        for (var i = 0; i < keys.length; i++) {
          obj[keys[i]] = rw.math.total(children, keys[i]);
        }
      },

      offlineMetrics: function (instanceId, data) {
        if (!('metrics' in this)) {
          this.loadMetrics(instanceId, data);
        }
        // todo. randomize data
      },

      offlineProtoMetrics: function (instanceId, data) {
        if (!('metrics' in this)) {
          this.loadProtoMetrics(instanceId, data);
        }
        // todo. randomize data
      },

      unsubscribe: function () {
        if (this.sockets.length > 0) {
          for (var i = 0; i < this.sockets.length; i++) {
            this.sockets[i].unsubscribe();
          }
          this.sockets.length = 0;
        }
      },

      detached: function () {
        this.unsubscribe();
        this.promise = null;
      }
    };
  });