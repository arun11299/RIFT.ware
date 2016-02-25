(function(window, angular) {
  angular.module('rwServicesPage')
  .directive('rwServicesPage', function() {

var servicesPageFn = function($scope, $rootScope, $timeout, vnfFactory, vcsFactory, portStateFactory, radio) {
    var self = this;
    var appChannel = radio.channel('appChannel');
    self.appChannel = appChannel;
    this.l10n = window.rw.ui.l10n;
    var listeners = [];
  $.when(vnfFactory.attached(), portStateFactory.attached()).done(function() {
      var aggregator = new rw.XFpathVnf(portStateFactory, vnfFactory.services, false);
      listeners.push(appChannel.on('port-state-update', function() {
        aggregator.populate();
        // let children know port stats are calculated
        $scope.$broadcast("port-state-vnf-aggregated");
      }, self));
      $timeout(function() {
        self.vnfUpdate($scope, vnfFactory.services);
      });

    });

  $.when(vcsFactory.attached(), portStateFactory.attached()).done(function() {
    var aggregator = new rw.XFpathVcs(portStateFactory, vcsFactory.sector, false);
    listeners.push(appChannel.on('port-state-update', function() {
      aggregator.populate();
    }, self));
  });


  rw.BaseController.call(this);

  };

angular.extend(servicesPageFn.prototype, {
  hasTrafSim : false,
  hasTrafGen : false,
  hasSaegw: false,
  hasMme: false,
  hasSlb: false,
  hasIOT: false,
  trafSimStarted: rw.ui.trafsimStarted,

  vnfUpdate: function(scope, services) {
    var self = this;
    _.each(services, function(service) {
      console.log(service);
      if (service.type == 'trafgen') {
        self.hasTrafGen = true;
      } else if (service.type == 'trafsimclient') {
        self.hasTrafSim = true;
      } else if (service.type == 'ltegwsim') {
        self.hasSaegw = true;
      } else if (service.type == 'ltemmesim') {
        self.hasMme = true;
      } else if (service.type == 'slbalancer') {
        self.hasSlb = true;
      } else if (service.type == 'cag') {
        self.hasCag = true;
      }  else if (service.type == 'premise_gw') {
        self.hasPgw = true;
      } else if (service.type == 'iot_army') {
        self.hasIOT = true;
        self.iotClientName = service.name;
      } else if (service.type == 'iot_server') {
        self.iotServerName = service.name;
      }
    });
  },

  trafsimUpdate: function() {
    if (this.$.trafsims && this.$.trafsims.trafsims) {
      // assumes specific order, need way to tell client from server in list
      this.trafsimClient = this.$.trafsims.trafsims[0];

      this.trafsimServer = this.$.trafsims.trafsims[1];
    }
  }
}
)

    return {
      restrict: 'AE',
      controllerAs: 'servicesPage',
      controller: servicesPageFn,
      templateUrl: '/modules/views/rw.services.page.tmpl.html',
      replace: true
    }

  })



})(window, window.angular);
