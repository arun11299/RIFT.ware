/**
 * Trafgen Dashboard Traffic Generator
 * @fileoverview Trafgen Dashboard Traffic Generator
 */
(function(window, angular) {
  "use strict";

  angular.module('trafgen')
    .directive('rwDashboardTrafficGenerator', function(vnfFactory, portStateFactory, radio, vcsFactory, trafgenFactory) {
      return {
        restrict: 'AE',
        templateUrl: '/modules/views/rw.trafgen-dashboard_traffic_generator.tmpl.html',
        replace: true,
        bindToController: true,
        scope: {
          dialSize: '@?',
          panelWidth: '@?',

        },
        controllerAs: 'dashboardTrafficGenerator',
        controller: function($scope, radio, $timeout) {
          var self = this;
          var appChannel = radio.channel('appChannel');
          self.appChannel = appChannel;
          var trafgenStats;

          self.mpps = 0;
          self.mppsMax = 0;
          self.gbps = 0;
          self.gbpsMax = 0;
          self.dialSize = self.dialSize || 280;
          self.trafgen = null;
          self.panelWidth = parseInt(this.panelWidth) || 200;
          self.g = rw.trafgen;

          self.gauges = [
            {label: 'Gbps', rate: self.gbps, max: self.gbpsMax, color: 'hsla(212, 57%, 50%, 1)', colorLight: 'rgba(55,124,200,0.7)', paused: 'false'},
            {label: 'Mpps', rate: self.mpps, max: self.mppsMax, color: 'hsla(260, 35%, 50%, 1)', colorLight: 'hsla(260, 35%, 50%, 0.7)', paused: 'false'}
          ];

          $.when(vcsFactory.attached(), trafgenFactory.attached(), vnfFactory.attached(), portStateFactory.attached()).done(function(a, b, c, d) {
            $timeout(function() {
              trafgenStats = new rw.XFpathVnf(portStateFactory, vnfFactory.services, false);
              _.each(vnfFactory.services, function(s) {
                if (s.type == 'trafgen') {
                  self.trafgen = s;
                  appChannel.trigger('trafGenService', s);
                }
              });
            });
          }).fail(function(e) {
            console.log('failed', e);
          });

          appChannel.on('port-state-update', function() {
            $timeout(function() {
              if (!self.trafgen) {
                return;
              }
              trafgenStats.populate();
              // Change back to 1000 for development;
              var rateMod = 1000;
              self.gbps = self.g.startedPerceived * (self.trafgen.tx_rate_mbps + self.trafgen.rx_rate_mbps) / rateMod;
              self.gbpsMax = (self.trafgen.speed * 2) / rateMod;
              self.mpps = self.g.startedPerceived * ((self.trafgen.tx_rate_pps + self.trafgen.rx_rate_pps) / 1000000);
              self.mppsMax = self.gbpsMax * 1.5; // rough math from Aiden based on an average 64 packet size

              self.gauges[0].rate = self.gbps;
              self.gauges[0].max = self.gbpsMax;
              self.gauges[0].paused = !self.g.startedPerceived;
              self.gauges[1].rate = self.mpps;
              self.gauges[1].max = self.mppsMax; // rough math from Aiden based on an average 64 packet size
              self.gauges[1].paused = !self.g.startedPerceived;
            });
          }, self);
        rw.BaseController.call(this);
        }
      };
    });

})(window, window.angular);
