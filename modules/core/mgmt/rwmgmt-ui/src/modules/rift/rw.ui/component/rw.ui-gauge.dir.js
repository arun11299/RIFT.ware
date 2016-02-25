require('../js/gauge.js');
(function(window, angular) {

  angular.module('uiModule')
    .directive('rwGauge', function() {
      return {
        restrict: 'AE',
        template: '<canvas></canvas>',
        replace: false,
        scope: {
          min: '@?',
          max: '@?',
          size: '@?',
          color: '@?',
          value: '@?',
          resize: '@?',
          isAggregate: '@?'
        },
        bindToController: true,
        controllerAs: 'gauge',
        controller: function($scope, $element) {
          var self = this;
          this.gauge = null;
          this.min = this.min || 0;
          this.max = this.max || 100;
          this.nSteps = 10;
          this.size = this.size || 300;
          this.color = this.color || 'hsla(212, 57%, 50%, 1)';
          this.valueFormat = {
            "int": 1,
            "dec": 0
          };
          this.isAggregate = this.isAggregate || false;
          this.resize = this.resize || false;

          if (this.format == 'percent') {
            this.valueFormat = {
              "int": 3,
              "dec": 0
            };
          }
          $scope.$watch(function() {
            return self.max;
          }, function(n, o) {
            renderGauge()
          })

          $scope.$watch(function() {
            return self.value;
          }, function() {
            if(self.gauge) {
              // w/o rounding gauge will unexplainably thrash round.
              self.gauge.setValue(Math.round(self.value));
            }
          });
          angular.element(document).ready(function(){
            renderGauge();
          })
          window.testme = renderGauge;
          function renderGauge(calcWidth) {
            if (self.max == self.min) {
              self.max = 100;
            }
            var range = self.max - self.min;
            var step = Math.round(range / self.nSteps);
            var majorTicks = [];
            for (var i = 0; i <= self.nSteps; i++) {
              majorTicks.push(self.min + (i * step));
            };
            var redLine = self.min + (range * 0.9);
            var config = {
              isAggregate: self.isAggregate,
              renderTo: angular.element($element).find('canvas')[0],
              width: calcWidth || self.size,
              height: calcWidth || self.size,
              glow: true,
              units: false,
              title: false,
              minValue: self.min,
              maxValue: self.max,
              majorTicks: majorTicks,
              valueFormat: self.valueFormat,
              minorTicks: 5,
              strokeTicks: false,
              highlights: [{
                from: self.min,
                to: redLine,
                color: self.color
              }, {
                from: redLine,
                to: self.max,
                color: 'hsla(8, 59%, 46%, 1)'
              }],
              colors: {
                plate: '#3b3b3b',
                majorTicks: '#ccc',
                minorTicks: '#ccc',
                title: '#ccc',
                units: '#fff',
                numbers: '#fff',
                needle: {
                  start: 'hsla(8, 59%, 46%, 1)',
                  end: 'hsla(8, 59%, 46%, 1)'
                }
              }
            };
            var updateSize = _.debounce(function() {
              config.maxValue = self.max;
              var clientWidth = self.parentNode.parentNode.clientWidth / 2;
              var calcWidth = (300 > clientWidth) ? clientWidth : 300;
              self.gauge.config.width = self.gauge.config.height = calcWidth;
              self.renderGauge(calcWidth);
            }, 500);
            if (self.resize) $(window).resize(updateSize)
            if (self.gauge) {
              self.gauge.updateConfig(config);
            } else {
              self.gauge = new Gauge(config);
              self.gauge.draw();
            }
          };

        },

      }
    })

})(window, window.angular);
