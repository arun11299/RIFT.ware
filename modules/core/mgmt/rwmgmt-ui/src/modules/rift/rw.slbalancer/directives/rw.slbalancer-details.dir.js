/**
 * Command Center Module
 * @fileoverview Command Center Module parent
 */
(function(window, angular) {
  "use strict";

  /**
   * <slbalancer-details>
   */
  var controller = function(radio, $scope, slbFactory) {
    this.showProtoStats = false;
    var self = this;
    var appChannel = radio.channel('appChannel');
    self.appChannel = appChannel;
    slbFactory.attached().done(function() {
      self.slb = slbFactory.slb;
    });
    appChannel.on("slb-update", function() {
      self.updatedMetrics();
    }, self);
    rw.BaseController.call(this);
  };

  controller.prototype = {

    updatedMetrics: function() {
      this.values = this.gatherValues(this.slb.dnsMetrics || this.slb.radiusMetrics);
    },

    //domReady: function() {
    //  this.$.barGraph.margin.left = 155;
    //},
    //
    gatherValues: function(metrics) {
      var self = this;
      var values = [];
      if (typeof(metrics) != 'undefined') {
        _.map(metrics, function (occurances, domain) {
          self.showProtoStats = true;
          values.push({label: domain, x: occurances});
        });
      }
      return values;
    }
  };

  angular.module('slbalancer')
    .directive('slbalancerDetails', function() {
      return {
        restrict: 'AE',
        templateUrl: '/modules/views/rw.slbalancer-details.tmpl.html',
        replace: true,
        controller : controller,
        controllerAs : 'slbalancerDetails',
        bindToController : true,
        scope : {
          service : '='
        }
      };
    });

})(window, window.angular);
