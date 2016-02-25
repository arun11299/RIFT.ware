(function(window, angular) {
  angular.module('trafsim')
    .directive('trafsimControl', function() {

      var controllerFn = function($scope, $element) {
        var self = this;
        self.orientation = self.orientation || 'vertical';
        self.created();

        $scope.$watch(function() {
            return self.g.rateActual;
          },
          function(oldVal, newVal) {
            if (newVal !== null) {
              self.globalRateChanged();
            }
          }
        );

        $scope.$watch(function() {
            return self.g.startedActual;
          },
          function(oldVal, newVal) {
            if (newVal !== null) {
              self.globalStartedChanged();
            }
          }
        );
      };

      controllerFn.prototype = {

        created: function() {
          this.g = rw.trafsim;
          this.updateButtonIcon();
        },

        globalStartedChanged: function() {
          this.g.startedPerceived = this.g.startedActual;
          this.updateButtonIcon();
        },

        globalRateChanged: function() {
          this.g.ratePerceived = this.g.rateActual;
        },

        toggleStarted: function() {
          // this should come back around and trigger globalStartedChanged
          this.g.startedPerceived = ! this.g.startedPerceived;
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
        }
      };

      return {
        restrict: 'AE',
        templateUrl: '/modules/views/rw.trafsim-control.tmpl.html',
        scope: {
          orientation: '@?'
        },
        bindToController: true,
        replace: true,
        controller: controllerFn,
        controllerAs: 'trafsimControl'
      };


    });
})(window, window.angular);
