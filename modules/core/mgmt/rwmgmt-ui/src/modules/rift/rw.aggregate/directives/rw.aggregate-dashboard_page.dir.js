module.exports = (function(window, angular) {
  "use strict";

  /**
   * <dashboard-page>
   */

  var controller = function($rootScope, $scope, vnfFactory, vcsFactory, portStateFactory, $timeout, radio, trafsimFactory, trafgenFactory) {

    var self = this;
    var appChannel = radio.channel('appChannel');
    this.oneTimeSync = function(prop, f) {
      var unregister = $scope.$watch(prop, function(newValue, oldValue, e) {
        f.call(self, newValue, oldValue, e);
        if (newValue != null) {
          unregister();
        }
      });
    };
    this.created();

    var listeners = [];
    trafgenFactory.attached();
    portStateFactory.attached().done(function() {

      var allAggregator = new rw.XFpathKitchenSink(portStateFactory, self);
      listeners.push(appChannel.on('port-state-update', function() {
        allAggregator.populate();
        self.fpathUpdate();
        self.aggregrateGauges[0].rate = self.gbps;
        self.aggregrateGauges[0].max = self.gbpsMax;
        self.aggregrateGauges[1].rate = self.mpps;
        self.aggregrateGauges[1].max = self.mppsMax;
        $scope.$apply();
      }, self));
    });

    $.when(vcsFactory.attached(), portStateFactory.attached()).done(function() {
      var aggregator = new rw.XFpathVcs(portStateFactory, vcsFactory.sector);
      listeners.push(appChannel.on('port-state-update', function() {
        $timeout(aggregator.populate.bind(aggregator));
      }, self));
    });

    vnfFactory.attached().done(function() {
      _.each(vnfFactory.services, function(service) {
        if (service.type === 'trafgen') {
          if (!self.hasTrafGen) {
            self.trafgen = service;
            self.hasTrafGen = true;
            self.hasTrafGenChanged();
          }
        }
        if (service.type === 'trafsimclient') {
          if (!self.hasTrafSim) {
            var trafsimService = service;
            self.trafsimColony = trafsimService.connector[0].interface[0].colonyId;
            self.hasTrafSim = true;
            self.hasTrafSimChanged();
            trafsimFactory.attached().then(function() {
              self.trafsim = trafsimFactory.trafsims[0];              
            });
          }
        }
      });
    });

    $scope.$watch(
      function() {
        return self.rate;
      },function() {
        self.rateChanged()
      });
    rw.BaseController.call(this);
  };

  controller.prototype = {

    toggleStarted: function() {
      this.started = ! this.started;
      if (this.hasTrafGen) {
        rw.trafgen.startedPerceived = this.started;
      }
      if (this.hasTrafSim) {
        rw.trafsim.startedPerceived = this.started;
      }
    },

    fpathUpdate: function() {
      var started = this.started || false;
      this.gbps = started * (this.tx_rate_mbps + this.rx_rate_mbps) / 1000;
      this.gbpsMax = (this.speed * 2) / 1000;
      this.bpsPercent = Math.round(100 * this.gbps / this.gbpsMax);
      this.mpps = started * ((this.tx_rate_pps + this.rx_rate_pps) / 1000000);
      this.mppsMax = this.gbpsMax * 1.5; // rough math from Aydin based on an average 64 packet size
    },

    created: function() {
      var self = this;
      this.aggregrateGauges = [
        {label: 'Gbps', rate: 0, max: 100, color: 'hsla(212, 57%, 50%, 1)',
          colorLight: 'rgba(55,124,200,0.7)'},
        {label: 'Mpps', rate: 0, max: 100, color: 'hsla(260, 35%, 50%, 1)',
          colorLight: 'hsla(260, 35%, 50%, 0.7)'}
      ];
      this.started = true;
      this.hasTrafGen = false;
      this.trafsim = null;
      this.trafgen = null;
      this.trafsimColony = null;
      this.hasTrafSim = false;
      this.gbps = 0;
      this.gbpsMax = 0;
      this.mpps = 0;
      this.mppsMax = 0;
      this.rate = rw.trafgen.ratePerceived;  // simple guess, but right 1/2 the time and cause less flicker
      this.tester = new rw.RateTimer(function(testRate) {
          self.rate = testRate;
        }, function() {
          self.started = false;
        }
      );

      // on very initial page load, we need to wait for server side to give us
      // initial state
      this.oneTimeSync(function() { return rw.trafgen.startedActual; },
        this.syncStarted.bind(this));
      this.oneTimeSync(function() { return rw.trafsim.startedActual; },
        this.syncStarted.bind(this));
      this.oneTimeSync(function() { return rw.trafgen.rateActual; },
        this.syncRate.bind(this));
      this.oneTimeSync(function() { return rw.trafsim.rateActual; },
        this.syncRate.bind(this));
    },

    hasTrafSimChanged: function() {
      if (typeof(this.hasTrafGen) == 'undefined') {
        return;
      }
      this.syncStarted();
      this.syncRate();
    },

    hasTrafGenChanged: function() {
      if (typeof(this.hasTrafSim) == 'undefined') {
        return;
      }
      this.syncStarted();
      this.syncRate();
    },

    syncStarted: function() {
      if (this.hasTrafSim && this.hasTrafGen) {
        this.started = rw.trafsim.startedPerceived && rw.trafgen.startedPerceived;
      } else if (this.hasTrafSim) {
        this.started = rw.trafsim.startedPerceived;
      } else if (this.hasTrafGen) {
        this.started = rw.trafgen.startedPerceived;
      }
    },

    syncRate: function() {
      if (this.hasTrafGen) {
        this.rate = rw.trafgen.ratePerceived;
      } else if (this.hasTrafSim) {
        this.rate = this.normalizeRate(rw.trafsim.ratePerceived);
      }
    },

    normalizeRate: function(rate) {
      return Math.min(100, Math.round((rate / rw.trafsim.maxRate) * 100));
    },

    testingChanged: function() {
      if (this.testing) {
        this.started = true;
        this.tester.start();
      } else {
        this.tester.stop();
      }
    },

    rateChanged: function() {
      if (this.hasTrafSim) {
        rw.trafsim.ratePerceived = Math.round((this.rate / 100) * rw.trafsim.maxRate);
      }
      if (this.hasTrafGen) {
        rw.trafgen.ratePerceived = this.rate;
      }
    },

    startedChanged: function() {
      if (this.hasTrafGen) {
        rw.trafgen.startedPerceived = this.started;
      }
      if (this.hasTrafSim) {
        rw.trafsim.startedPerceived = this.started;
      }
    }
  };

  angular.module('rw.aggregate')
    .directive('dashboardPage', function() {
      return {
        restrict: 'AE',
        templateUrl: '/modules/views/rw.aggregate-dashboard_page.tmpl.html',
        scope: {

        },
        controller: controller,
        controllerAs: 'dashboardPage',
        bindToController: true,
        replace: true
      };
    });
})(window, window.angular);
