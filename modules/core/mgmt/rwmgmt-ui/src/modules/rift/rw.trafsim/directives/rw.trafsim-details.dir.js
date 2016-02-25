/**
 * Command Center Module
 * @fileoverview Command Center Module parent
 */
(function(window, angular) {
  "use strict";

  /**
   * <trafsim-details> -
   */
  var controller = function($scope, radio, $timeout, trafsimFactory) {
    var self = this;
    var appChannel = radio.channel('appChannel');
    self.appChannel = appChannel;
    self.created();
    var listeners = [];
    listeners.push(
      appChannel.on('trafsim-update', function() {
        $timeout(self.trafsimChanged.bind(self));
      }, self)
    );
    $scope.$watch(function() {
        return self.service;
      },
      function() {
        if (typeof(self.service) != 'undefined') {
          trafsimFactory.attached().done(function() {
            if (self.service.type === 'trafsimclient') {
              self.trafsim = trafsimFactory.findFirstClient();
            } else {
              self.trafsim = trafsimFactory.findFirstServer();
            }
            $timeout(self.trafsimChanged.bind(self));
          });
        }
      }
    );
    $scope.$on('$stateChangeStart', function() {
      // do not detach from model, it's probably still in use by other
      // components
      _.each(listeners, function(listener) {
        listener.cancel();
      });
      listeners.length = 0;
    });
  };

  controller.prototype = {

    created: function() {
      this.values = [];
    },

    timerLabel: function(value) {
      var v_valueArray = value.split("_");
      if (v_valueArray[1] == "0ms"){
        return "< 25ms";
      } else if (v_valueArray[1] == "3000ms") {
        return ">= 3000ms";
      } else {
        return v_valueArray[1];
      }
    },

    trafsimChanged: function() {
      if (typeof this.trafsim == 'undefined' || typeof this.trafsim.cumulative == 'undefined') {
        return;
      }
      if (this.trafsim.isClient) {
        this.trafsim.sent_calls = this.trafsim.call_rate;
        this.trafsim.c_sent_calls = this.trafsim.cumulative.call_rate;
        this.trafsim.recieved_calls = this.trafsim.c_recieved_calls = 0;
      } else {
        this.trafsim.recieved_calls = this.trafsim.call_rate;
        this.trafsim.c_recieved_calls = this.trafsim.cumulative.call_rate;
        this.trafsim.sent_calls = this.trafsim.c_sent_calls = 0;
      }
      this.trafsim.total_calls = this.trafsim.sent_calls + this.trafsim.recieved_calls;

      var timers = _.filter(_.keys(this.trafsim.timers), function(timer) {
        return timer != 'rwappmgr-instance';
      });
      if (this.values.length != timers.length) {
        this.values = new Array(timers.length);
      }
      for (var i = 0; i < this.values.length; i++) {
        this.values[i] = this.values[i] || {};
        this.values[i].label = this.timerLabel(timers[i]);
        this.values[i].x = this.trafsim.timers[timers[i]];
      }

      this.broadcast('bargraph-update');
    }

  };

  angular.module('trafsim')
    .directive('trafsimDetails', function() {
      return {
        restrict: 'AE',
        templateUrl: '/modules/views/rw.trafsim-details.tmpl.html',
        replace: true,
        controller : controller,
        controllerAs : 'trafsimDetails',
        bindToController: true,
        scope : {
          service : '='
        }
      };
    });

})(window, window.angular);



