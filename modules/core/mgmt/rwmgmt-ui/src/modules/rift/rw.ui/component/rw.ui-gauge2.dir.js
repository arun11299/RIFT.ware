require('../js/gauge2.js');
(function(window, angular) {

  angular.module('uiModule')
    .directive('rwGauge2', function() {
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
        controllerAs: 'gauge2',
        controller: function($scope, $element) {
          var self = this;
          this.gauge = null;
          this.min = this.min || 0;
          this.max = this.max || 100;
          this.nSteps = 14;
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
              self.max = 14;
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
              glow: false,
              units: false,
              title: false,
              minValue: self.min,
              maxValue: self.max,
              majorTicks: majorTicks,
              valueFormat: self.valueFormat,
              minorTicks: 0,
              strokeTicks: false,
              highlights: [],
              colors: {
                plate: 'rgba(0,0,0,0)',
                majorTicks: '#ccc',
                minorTicks: '#ccc',
                title: '#ccc',
                units: '#fff',
                numbers: '#fff',
                needle: {
                  start: 'rgba(255, 255, 255, 1)',
                  end: 'rgba(255, 255, 255, 1)'
                }
              }
            };
            var min = config.minValue;
            var max = config.maxValue;
            var N = 1000;
            var increment = (max - min) / N;
            for (i = 0; i < N; i++) {
              var temp_color = 'rgb(19, 154, 233)';
              if (i  > 0.5714 * N && i <= 0.6428 * N) {
                temp_color = 'rgb(17,138,208)';
              } else if (i >= 0.6428 * N && i < 0.7142 * N) {
                temp_color = 'rgb(15,122,183)';
              } else if (i >= 0.7142 * N && i < 0.7857 * N) {
                temp_color = 'rgb(13,106,159)';
              } else if (i >= 0.7857 * N && i < 0.8571 * N) {
                temp_color = 'rgb(11,92,136)';
              } else if (i >= 0.8571 * N && i < 0.9285 * N) {
                temp_color = 'rgb(10,77,114)';
              } else if (i >= 0.9285 * N) {
                temp_color = 'rgb(8,63,94)';
              }
              config.highlights.push({
                from:i * increment,
                to:increment * (i + 2),
                color: temp_color
              })


            }
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
