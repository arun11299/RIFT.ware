(function(window, angular) {

  // Belongs in a central location
  function BaseController() {

    var self = this;

    // handles factories with detach/cancel methods and listeners with cancel method
    this.$scope.$on('$stateChangeStart', function() {
      var properties = Object.getOwnPropertyNames(self);
      properties.forEach(function(key) {

        var propertyValue = self[key];

        if (propertyValue) {
          if (Array.isArray(propertyValue)) {
            propertyValue.forEach(function(item) {
              if (item.cancel && typeof item.cancel == 'function') {
                item.cancel();
              }
            });
            propertyValue.length = 0;
          } else {
            if (propertyValue.detached && typeof propertyValue.detached == 'function') {
              propertyValue.detached();
            } else if (propertyValue.cancel && typeof propertyValue.cancel == 'function') {
              propertyValue.cancel();
            }
          }
        }
      });

      // call in to do additional cleanup
      if (self.doCleanup && typeof self.doCleanup == 'function') {
        self.doCleanup();
      }
    });
  };

  angular.module('ue')
    .directive('rwDashboardUePanel', ['ueFactory', 'radio', rwDashboardUePanel]);

    function rwDashboardUePanel() {

      function Controller(ueFactory, radio, $timeout, $scope) {
        var self = this;
        var appChannel = radio.channel('appChannel');
        // self.appChannel = appChannel;
        self.listeners = [];
        self.ueFactory = ueFactory;
        self.$scope = $scope;

        this.l10n = window.rw.ui.l10n;
        self.listeners.push(appChannel.on('ue-update', function() {
          $timeout(function() {
            self.updateMetrics(self.ueFactory);
          });
        }, self));

        self.ueFactory.attached();

        BaseController.call(this);
      };

      Controller.prototype = Object.create(BaseController.prototype, {
        mmeGauges: {
          configurable: true,
          enumerable: true,
          value: [
            {label: 'Calls Transmitted', rate: 0, max: 100, color: 'hsla(212, 57%, 50%, 1)', colorLight: 'rgba(55, 124, 200, 0.7)'},
            {label: 'Calls Received', rate: 0, max: 100, color: 'hsla(212, 57%, 50%, 1)', colorLight: 'rgba(55, 124, 200, 0.7)'},
          ]
        },
        saegwGauges: {
          configurable: true,
          enumerable: true,
          value: [
            {label: 'Calls Transmitted', rate: 0, max: 100, color: 'hsla(212, 57%, 50%, 1)', colorLight: 'rgba(55, 124, 200, 0.7)'},
            {label: 'Calls Received', rate: 0, max: 100, color: 'hsla(212, 57%, 50%, 1)', colorLight: 'rgba(55, 124, 200, 0.7)'},
          ]
        }
      });

      Controller.prototype.updateMetrics = function(ueFactory) {
        this.mmeGauges[0].rate = (parseInt(ueFactory['mme'].acceptedTxCalls));
        this.mmeGauges[1].rate = (parseInt(ueFactory['mme'].acceptedRxCalls));
        this.saegwGauges[0].rate = (parseInt(ueFactory['saegw'].acceptedTxCalls));
        this.saegwGauges[1].rate = (parseInt(ueFactory['saegw'].acceptedRxCalls));
      };

      return {
        restrict: 'AE',
        templateUrl: '/modules/views/rw.ue-dashboard_ue_panel.tmpl.html',
        scope: {
          hasMme: '=',
          hasSaegw: '='
        },
        bindToController: true,
        controllerAs: 'rwDashboardUePanel',
        controller: Controller,
        replace: true
      };
    };
})(window, window.angular);
