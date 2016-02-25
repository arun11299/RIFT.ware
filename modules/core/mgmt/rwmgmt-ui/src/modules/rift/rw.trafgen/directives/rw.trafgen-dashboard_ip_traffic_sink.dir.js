/**
 * Trafgen Dashboard Traffic Sink
 * @fileoverview Trafgen Dashboard Traffic Sink
 * <rw-dashboard-ip-traffic-sink>
 */

(function(window, angular) {
  "use strict";

  var self = this;

  angular.module('trafgen')
    .directive('rwDashboardIpTrafficSink', ['vnfFactory', 'radio', rwDashboardIpTrafficSink]);

  function rwDashboardIpTrafficSink() {

    function Controller(vnfFactory, radio, $timeout, $scope) {
      var self = this;
      var appChannel = radio.channel('appChannel');
      self.appChannel = appChannel;
      self.listeners = [];
      self.vnfFactory = vnfFactory;
      self.$scope = $scope;
      self.trafsink = null;
      self.g = rw.trafgen;
      self.vnfFactory.attached().done(function() {
        _.each(vnfFactory.services, function(s) {
          if (s.type == 'trafsink') {
            self.trafsink = s;
          }
        });
      })
      .fail(function(e) {
        console.log('failed', e);
      });

      self.listeners.push(appChannel.on('port-state-update', function() {
        if (!self.trafsink) {
          return;
        }

        $timeout(function() {
          self.updateMetrics(self.trafsink, self.g);
        });
      }, self));
      rw.BaseController.call(this);
    };

    Controller.prototype = Object.create(rw.BaseController.prototype, {
      gauges: {
        configurable: true,
        enumerable: true,
        value: [
          {label: 'Gbps', rate: 0, max: 0, color: 'hsla(212, 57%, 50%, 1)', colorLight: 'rgba(55,124,200,0.7)', paused: 'false'},
          {label: 'Mpps', rate: 0, max: 0, color: 'hsla(260, 35%, 50%, 1)', colorLight: 'hsla(260, 35%, 50%, 0.7)', paused: 'false'}
        ]
      }
    });

    Controller.prototype.updateMetrics = function(service, g) {
      var self = this;
      // Change back to 1000 for development;
      var rateMod = 1000;
      self.gbps = self.g.startedPerceived * (self.trafsink.tx_rate_mbps + self.trafsink.rx_rate_mbps) / rateMod;
      self.gbpsMax = (self.trafsink.speed * 2) / rateMod;
      self.mpps = self.g.startedPerceived * ((self.trafsink.tx_rate_pps + self.trafsink.rx_rate_pps) / 1000000);
      self.mppsMax = self.gbpsMax * 1.5; // rough math from Aiden based on an average 64 packet size

      self.gauges[0].rate = self.gbps;
      self.gauges[0].max = self.gbpsMax;
      self.gauges[0].paused = !self.g.startedPerceived;
      self.gauges[1].rate = self.mpps;
      self.gauges[1].max = self.mppsMax; // rough math from Aiden based on an average 64 packet size
      self.gauges[1].paused = !self.g.startedPerceived;
    };

    return {
      restrict: 'AE',
      templateUrl: '/modules/views/rw.trafgen-dashboard_ip_traffic_sink.tmpl.html',
      replace: true,
      bindToController: true,
      scope: {
        dialSize: '@?',
        panelWidth: '@?',
        class: '@?'
      },
      controllerAs: 'dashboardIpTrafficSink',
      controller: Controller
    };
  };
})(window, window.angular);
