
module.exports = (function(window, angular) {

  var controller = controllerFn;

  angular.module('trafgen')
    .directive('trafgenControl', function() {

      return {
        restrict: 'AE',
        scope: {
          testModeEnabled: '=?',
          orientation: '@?'
        },
        templateUrl: '/modules/views/rw.trafgen-control.tmpl.html',
        replace:true,
        bindToController: true,
        replace: true,
        controllerAs: 'trafgenControl',
        controller: controllerFn
      }
    });

  var controllerFn = function($rootScope, $scope, $element, $timeout) {
     var self = this;
     self.orientation = self.orientation || 'vertical';
     self.created();
     console.log('Created ratePerceived', self.g.ratePerceived);
     console.log('Created rateActual', self.g.rateActual);
//There should be a better way of doing this. Circular watchers with rw.trafgen-controller.dir.js
      $scope.$watch(function() {
        return rw.trafgen.startedActual;
      },  function() {
        $timeout(function() {
          self.globalStartedChanged();
        })

       });
      // $scope.$watch(function() {
      //   return rw.trafgen.rateActual;
      // }, function() {
      //   $timeout(function() {
      //     self.globalRateChanged();
      //   });
      // });
      // $scope.$watch(function() {
      //   return rw.trafgen.packetSizeActual;
      // }, function() {
      //   self.globalPacketSizeChanged();
      // });

  }

  controllerFn.$inject = ['$rootScope', '$scope', '$element','$timeout'];

  controllerFn.prototype = {
    observe: {
      'g.startedActual': 'globalStartedChanged',
      'g.rateActual': 'globalRateChanged',
      'g.packetSizeActual': 'globalPacketSizeChanged'
    },

    created: function() {
      this.g = rw.trafgen;
      this.updateButtonIcon();
    },

    attached: function() {
      this.orientation = this.classList.contains('horizontal') ? 'horizontal' : 'vertical';
    },

    globalStartedChanged: function() {
      this.g.startedPerceived = this.g.startedActual;
      this.updateButtonIcon();
    },

    // globalRateChanged: function() {
    //   console.log('Updated rateActual', this.g.rateActual);
    //   this.g.ratePerceived = this.g.rateActual;
    // },

    // globalPacketSizeChanged: function() {
    //   this.g.packetSizePerceived = this.g.packetSizeActual;
    // },
    testFn: function() {
      console.log('wut')
    },
    toggleStarted: function() {
      this.g.startedPerceived = !this.g.startedPerceived;
      this.updateButtonIcon();

    },

    updateButtonIcon: function() {
      if (this.g.startedPerceived != this.g.startedActual) {
        this.buttonIcon = 'icon-loading';
        this.loading = true;
      } else if (this.g.startedPerceived) {
        this.buttonIcon = 'icon-power-on';
        this.loading = false;
      } else {
        this.buttonIcon = 'icon-power-off';
        this.loading = false;
      }
    },

    detached: function() {
      this._propertyObserver.disconnect_();
    }
  }
})(window, window.angular);
