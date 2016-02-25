
(function(window, angular) {
  var smooth = require('../js/smoothie.js');
  // <canvas width="scrollgraph.width" height="scrollgraph.height"></canvas>
  try{
    angular.module('uiModule');
  } catch(e) {
    angular.module('uiModule',['ui.bootstrap','ui.codemirror'])
  }

  angular.module('uiModule')
    .directive('rwScrollGraph', function() {
      return {
        restrict: 'AE',
        replace:true,
    template: '<div class="scroll-graph">' +
      '<div>' +
        '<div>' +
          '<canvas  height="{{scrollGraph.height}}" width="{{scrollGraph.width}}"></canvas>'+
        '</div>'+
      '</div>' +
    '</div>',
        scope: {
          min: '@?',
          max: '@?',
          lineColor:'@?',
          lineColor2:'@?',
          scrollWidth: '@?width',
          scrollHeight:'@?height',
          strokeColor:'@?',
          strokeColor2:'@?',
          paused: '@?',
          width: '@?',
          labels:'@?',
          value:'@?',
          height: '@?'
        },
        bindToController: true,
        controllerAs: 'scrollGraph',
        controller: function($scope) {

          var self = this;
          this.min = 0;
          this.lineColor = this.lineColor|| 'hsla(0, 0%, 13%, 1)';
          this.lineColor2 = 'hsla(0, 50%, 13%, 1)';
          this.width = parseInt(this.width) || 100;
          this.height = parseInt(this.height) || 80;
          this.strokeColor = this.strokeColor || 'hsla(0, 0%, 13%, 0.7)';
          this.strokeColor2 = 'hsla(0, 50%, 13%, 0.7)';
          this.paused = (this.paused == 'false' || !this.paused) ? false : true;
          this.labels = true;
          var updateSize = _.debounce(function() {
              //          console.log('resizing',self.offsetWidth, this.width)
              self.width = self.offsetWidth - 30;
              //          self.attached();
              //          self.pausedChanged();
            })
            //        $(window).resize(updateSize)
        },
        link: function(s,e,a,c) {
          var init = init.bind(c);
          var valueChanged = valueChanged.bind(c);
          var maxChanged = maxChanged.bind(c);
          var value2Changed = value2Changed.bind(c);
          var pausedChanged = pausedChanged.bind(c);

          s.$watch(function() {
            return c.value;
          }, valueChanged )
             s.$watch(function() {
            return c.max;
          }, maxChanged )

                   s.$watch(function() {
            return c.paused;
          }, pausedChanged )
          init();

        function init() {
        var self = this;
        this.series = new smooth.TimeSeries();
        this.chart = new smooth.SmoothieChart({
          minValue : this.min,
          maxValue : this.max,
          labels: {
            disabled : ! this.labels
          },
          grid: {
            fillStyle: '#3b3b3b',
            strokeStyle: '#5e5e5e',
            verticalSections: 4,
            millisPerLine: 5000,
            sharpLines: true
          },
          millisPerPixel: 100
        });
        this.chartStyle = {
          strokeStyle: this.lineColor,
          fillStyle: this.strokeColor,
          lineWidth: 2
        };
        this.chart.addTimeSeries(this.series, this.chartStyle);

        if (this.value2 != undefined) {
          this.series2 = new smooth.TimeSeries();
          this.chartStyle2 = {
            strokeStyle: this.lineColor2,
            fillStyle: this.strokeColor2,
            lineWidth: 2
          };
          this.chart.addTimeSeries(this.series2, this.chartStyle2);

        }

        this.chart.streamTo(angular.element(e).find('canvas')[0], 2000);
        // with no new values coming in, chart goes blank.
        var self = this;
        this.keepAlive = setInterval(function() {
          valueChanged();
          if (self.series2) {
            self.value2Changed();
          }
        }, 5000);
        this.chart.stop();
        this.chart.start();
      }

 function maxChanged(){
        if(this.chart) {
          this.chart.options.maxValue = this.max;
          this.chart.render();
        }
      }
      function pausedChanged() {

        this.paused = (this.paused == 'false' || !this.paused) ? false : true;
        if (this.chart) {
          if (this.paused) {
            this.chart.stop();
          } else {
            this.chart.start();
          }
        }
      }

      function valueChanged() {
        if (this.series) {
          var t = new Date().getTime();
          this.series.append(t, this.value);
        }
      }

function value2Changed() {
        if (this.series2) {
          var t = new Date().getTime();
          this.series2.append(t, this.value2);
        }
      }

        } //end Link

      }
    })






  //Modified start function to enable rendering of first frame only.
  // window.SmoothieChart.prototype.start = function(stop) {
  //   if (this.frame) {
  //     // We're already running, so just return
  //     return;
  //   }
  //   if (stop) {
  //     this.render();
  //   } else {
  //     // Renders a frame, and queues the next frame for later rendering
  //     var animate = function() {
  //       this.frame = SmoothieChart.AnimateCompatibility.requestAnimationFrame(function() {
  //         this.render();
  //         animate();
  //       }.bind(this));
  //     }.bind(this);
  //     animate();
  //   }
  // };

})(window, window.angular);
