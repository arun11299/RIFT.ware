/**
 * LTE Sim Summary
 */
(function(window, angular) {

  "use strict";

  /**
   * <lte-details>
   */
  var controller = function($scope, ueFactory, radio) {
    var self = this;
    var appChannel = radio.channel('appChannel');
    self.appChannel = appChannel;
    self.ueFactory = ueFactory;
    $scope.theme = rw.theme;
    self.listeners = [];
    $scope.$watch(
      function() {
        return self.service;
      },
      function() {
        ueFactory.attached().done(function() {
          self.ltesim = ueFactory.ueForService(self.service);
          //self.serviceChanged();
        });
      }
    );
    self.listeners.push(appChannel.on('ue-update', function() {
      self.ltesimChanged();
      self.ltesimRestChanged();
    }, self));
    rw.BaseController.call(this);
  };

  controller.prototype = {
    maxCallRate : 200000,
    maxMsgRate : 600000,

    ltesimChanged: function() {
      if (!('metrics' in this.ltesim) || !('counters' in this.ltesim.metrics)) {
        return;
      }
      var self = this;
      var counters = [];
      for (var service in this.ltesim.metrics.counters) {
        var container = this.ltesim.metrics.counters[service];
        for (var counterProp in container) {
          var counter = container[counterProp];
          if (!jQuery.isEmptyObject(counter)) {
            //var data = JSON.stringify(counter, null, '  ');
            var children = self.getChildren(counter);
            counters.push({name : counterProp, children : children});
          }
        }
      }
      this.counters = counters;
    },

    ltesimRestChanged: function() {
      if (!('restMetrics' in this.ltesim)) {
        return;
      }

      this.restMetrics = this.getChildren(this.ltesim.restMetrics);
    },

    getChildren: function(obj) {
      var children = [];
      for (var prop in obj) {
        if (typeof(obj[prop]) == 'object') {
          var grandChildren = this.getChildren(obj[prop]);
          children.push({name : prop, children : grandChildren})
        } else {
          children.push({name : prop, value : obj[prop]});
        }
      }
      return children;
    }
  };

  angular.module('ue')
    .directive('lteDetailsProperty', function(RecursionHelper) {
      return {
        restrict: 'AE',
        templateUrl: '/modules/views/rw.ue-lte_details_property.tmpl.html',
        replace: true,
        scope : {
          prop : '='
        },
        compile: function(element) {
          // Use the compile function from the RecursionHelper,
          // And return the linking function(s) which it returns
          return RecursionHelper.compile(element);
        }
      };
    })
    .directive('lteDetails', function() {
      return {
        restrict: 'AE',
        templateUrl: '/modules/views/rw.ue-lte_details.tmpl.html',
        replace: true,
        controller : controller,
        controllerAs: 'lteDetails',
        bindToController: true,
        scope : {
          service : '='
        }
      };
    });

})(window, window.angular);
