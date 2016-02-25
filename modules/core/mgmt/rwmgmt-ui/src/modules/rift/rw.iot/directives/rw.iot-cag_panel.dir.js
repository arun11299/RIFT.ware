(function(angular, window) {
  "use strict";

  var controllerFn = function(radio, $rootScope, $scope, $timeout, iotSlbFactory, vnfFactory) {
    var self = this;
    var appChannel = radio.channel('appChannel');
    self.appChannel = appChannel;
    self.created();

    var listeners = [];
    $.when(iotSlbFactory.attached(), vnfFactory.attached()).done(function() {

      listeners.push(appChannel.on("iotSlb-update", function() {
        self.slbUpdateMetrics(iotSlbFactory.metricsByFpath["cag"]);
      }, self));
    });



    var cancelUpdate = $scope.$on('port-state-vnf-aggregated', function() {
      $timeout(self.vnfUpdate.bind(self, vnfFactory.services));
      cancelUpdate();
    });
      rw.BaseController.call(this);
  };

  angular.extend(controllerFn.prototype, {
    created: function() {
      // this.slbIncoming = [
      //   {label: 'Gbps', rate: 0, max: 100, color: 'hsla(212, 57%, 50%, 1)', colorLight: 'rgba(55,124,200,0.7)'},
      //   {label: 'Mpps', rate: 0, max: 100, color: 'hsla(260, 35%, 50%, 1)', colorLight: 'hsla(260, 35%, 50%, 0.7)'}
      // ];
      // this.slbOutgoing = [
      //   {label: 'Gbps', rate: 0, max: 100, color: 'hsla(212, 57%, 50%, 1)', colorLight: 'rgba(55,124,200,0.7)'},
      //   {label: 'Mpps', rate: 0, max: 100, color: 'hsla(260, 35%, 50%, 1)', colorLight: 'hsla(260, 35%, 50%, 0.7)'}
      // ];
      // this.l10n = rw.ui.l10n;
    },

    slbUpdateMetrics:function(m) {
      // this.slbIncoming[0].rate = (parseInt(m['clnt-fwd-rate-mbps']) + parseInt(m['clnt-rev-rate-mbps'])) / 1000;
      // this.slbIncoming[1].rate = (parseInt(m['clnt-fwd-rate-pps']) + parseInt(m['clnt-rev-rate-pps'])) / 1000000;
      // this.slbOutgoing[0].rate = (parseInt(m['srvr-fwd-rate-mbps']) + parseInt(m['srvr-rev-rate-mbps'])) / 1000;
      // this.slbOutgoing[1].rate = (parseInt(m['srvr-fwd-rate-pps']) + parseInt(m['srvr-rev-rate-pps'])) / 1000000;
      this.stats = m;
    },

    vnfUpdate: function(services) {
      var totMbps = function(tot, iface) {slb
        return tot + parseInt(iface.speed);
      };
      var self = this;
      _.each(services, function(service) {
        if (service.type === 'slbalancer') {
          var maxGbpsDuplex = (service.connector[0].interface.reduce(totMbps, 0) * 2) / 1000;
          self.slbIncoming[0].max = self.slbOutgoing[0].max = maxGbpsDuplex;
          self.slbIncoming[1].max = self.slbOutgoing[1].max = maxGbpsDuplex * 1.5;
          return false;
        }
     });
    }
  });

  angular.module('rw.iot')
    .directive('iotCagPanel', function() {
      return {
        restrict: 'AE',
        bindToController: true,
        controllerAs: 'iotCagPanel',
        controller: controllerFn,
        templateUrl: '/modules/views/rw.iot-cag_panel.tmpl.html',
        replace: true
      }
    });

})(window.angular, window);
