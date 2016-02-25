/**
 * Command Center Module
 * @fileoverview Command Center Module parent
 */
(function(window, angular) {
  "use strict";

  angular.module('loadbal')
    .directive('loadbalSummary', function(loadbalFactory) {
      return {
        restrict: 'AE',
        templateUrl: '/modules/views/rw.loadbal-summary.tmpl.html',
        replace: true,
        controller : function($scope, $timeout, radio, loadbalFactory) {
          var self = this;
          var appChannel = radio.channel('appChannel');
          self.$scope = $scope;
          self.appChannel = appChannel;
          self.loadbalFactory = loadbalFactory;

          self.listeners = [];

          self.$scope.$watch('service', function() {
            if (typeof(self.$scope.service) === 'undefined') {
              return;
            }

            self.listeners.push(appChannel.on('loadbal-update', function() {
              $timeout(function() {
                $scope.aggregateData = loadbalFactory.sum;
              });
            }, self));

            // ingress always first???
            self.$scope.ingressConnector = self.$scope.service.connector[0];
            self.$scope.egressConnector = self.$scope.service.connector[1];
            var colony = self.$scope.ingressConnector.interface[0].colonyId;
            self.loadbalFactory.attached(colony);

            rw.BaseController.call(self);
          });
        },
        scope : {
          service : '='
        }
      };
    });

})(window, window.angular);
