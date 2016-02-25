0/**
 * Command Center Module
 * @fileoverview Command Center Module parent
 */
module.exports = (function(window, angular) {
  "use strict";

  /**
   * <trafgen-details> -
   */
  var controller = function($scope, $timeout, radio, vcsFactory, trafgenFactory) {
    var self = this;
    var appChannel = radio.channel('appChannel');
    self.appChannel = appChannel;
    self.created();
    this.$scope = $scope;
    appChannel.on("trafgen-update", function() {
      $timeout(self.update.bind(self));
    }, self);
    $scope.$watch(function() {
        return self.service;
      },
      function() {
        if (typeof(self.service) != 'undefined') {
          self.serviceChanged();
        }
      }
    );
    this.trafgenModel = trafgenFactory;

    this.broadcast = function(event_name) {
      $scope.$broadcast(event_name);
    };
    rw.BaseController.call(this);
  };

  controller.prototype = {

    created: function() {
      this.values = [];
      this.showAdditionalMetrics = false;
    },

    serviceChanged: function() {
      this.colony = this.service.connector[0].interface[0].colonyId;
      this.isGenerator = (this.service.type === 'trafgen');
    },

    update: function() {
      if (!this.colony) {
        return;
      }
      var self = this;
      this.info = this.trafgenModel.info[this.colony];
      // Generatator

      var showAdditionalMetrics = false;
      if (this.isGenerator && this.info && 'latency-distribution' in this.info) {
        var latency = this.info['latency-distribution'];
        if (this.values.length != latency.length) {
          this.values = new Array(latency.length);
        }
        for (var i = 0; i < this.values.length; i++) {
          showAdditionalMetrics = true;
          this.values[i] = this.values[i] || {};
          this.values[i].label = this.timerLabel(latency[i]);
          this.values[i].x = latency[i].packets;
        }

        this.broadcast('bargraph-update');
      }

      // first time bargraph is shown from hidden state, need to trigger layout
      if (showAdditionalMetrics && !this.showAdditionalMetrics) {
        this.showAdditionalMetrics = true;
        _.debounce(self.broadcast.bind(self, 'bargraph-resize'), 1000);
      }
    },

    timerLabel: function(latency) {
      if (latency['range-start'] == 0) {
        return '< ' + latency['range-end'] + 'us';
      }
      if (latency['range-end'] == 0) {
        return '> ' + latency['range-start'] + 'us';
      }
      return '' + latency['range-start'] + ' - ' + latency['range-end'] + 'us';
    }
  };

  angular.module('trafgen')
    .directive('trafgenDetails', function() {
      return {
        restrict: 'AE',
        templateUrl: '/modules/views/rw.trafgen-details.tmpl.html',
        replace: true,
        controller : controller,
        controllerAs : 'trafgenDetails',
        bindToController: true,
        scope : {
          service : '='
        }
      };
    });

})(window, window.angular);



