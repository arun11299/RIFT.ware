(function(window, angular) {

  /**
   * <rw-bargraph>
   */
  angular.module('uiModule')
    .directive('rwBargraph', function() {
      return {
        restrict: 'AE',
        replace: true,
        controller: controllerFn,
        template: '<div><svg class="bargraph"></svg></div>',
        controllerAs: 'rwBarGraph',
        scope: {
          leftMargin: '@?',
          values: '=',
          xMin: '@',
          xMax: '@',
          width: '@',
          height: '@'
        },
        bindToController: true,
        link: function(scope, element, attr) {
          var self = scope.rwBarGraph;
          self.created(scope.rwBarGraph.leftMargin);
          angular.element(document).ready(function() {
            self.domReady.call(self, element[0])
          });
          scope.$watch(function(){
            return self.values;
          }, function(){
            self.valuesChanged()
          });
        }
      }
    });

  function controllerFn($scope, $element, $timeout) {
    this.element = $element[0];
    $scope.$on('bargraph-resize', this.resize.bind(this));
    $scope.$on('bargraph-update', this.update.bind(this));
    // RIFT-6728 - navigating to/fro will cause widget to collapsed
    $timeout(this.resize.bind(this), 500);
  }

  controllerFn.prototype = {
    color: 'rgba(73, 189, 238, 1)',
    xMin: 0,
    xMax: 500,

    created: function(leftMargin) {
      var left = parseInt(leftMargin || "55");
      this.margin = {
        top: 20,
        right: 100,
        bottom: 20,
        left: left
      };
      this.xDataMargin = 80;
      this.yDataMargin = 40;

      this.labels = [];
    },

    domReady: function(e) {
      var self = this;
      console.log(self)
      self.xValue = function(d) {
        return d.x;
      };
      self.yValue = function(d) {
        return d.label;
      };

      this.calculateSizes(e);
      // add the graph canvas to the body of the webpage
      self.svg = d3.select($(e).find('svg')[0]);
      self.plot = self.svg
        .append("g")
        .attr("transform",
          "translate(" + self.margin.left + "," + self.margin.top + ")");

      self.dataG = this.plot
        .append("rect")
        .attr('class', 'bargraph-data');

      self.yAxisG = self.plot.append("g")
        .attr("class", "y bargraph-axis");

      self.sizeToFit();
      self.valuesChanged();

      window.addEventListener("resize", _.debounce(self.resize.bind(self), 100));
    },

    resize: function() {
      this.calculateSizes(this.element);
      this.sizeToFit();
      this.valuesChanged();
    },

    calculateSizes: function(e) {
      var self = this;
      var dim = e.getBoundingClientRect();      
      this.height = this.height || Math.floor(dim.height);
      this.width = this.width || Math.floor(dim.width);

      this.plotWidth = this.width - this.margin.left - this.margin.right;
      this.plotHeight = this.height - this.margin.top - this.margin.bottom;

      this.xScale = d3.scale.linear().range([0, this.plotWidth - 30]);
      this.xMap = function(d) {
        return self.xScale(self.xValue(d));
      };
      this.xScale.domain([this.xMin, this.xMax]);
      this.xWidth = function(d) {
        return self.xMap(d);
      };

      this.yScale = d3.scale.ordinal().rangeRoundBands([0, this.plotHeight]);
      this.yMap = function(d) {
        return self.yScale(self.yValue(d));
      };
      this.yAxis = d3.svg.axis().scale(this.yScale).orient("left").tickSize(0);
      this.yScale.domain(this.labels);
    },

    sizeToFit: function() {
      var self = this;
      var w = this.plotWidth + self.margin.right + self.margin.left;
      var h = this.plotHeight + self.margin.top + self.margin.bottom;
      this.svg
        .attr("width", w)
        .attr("height", h);

      this.yAxisG.call(this.yAxis);

      this.dataG
        .attr("width", w - this.xDataMargin)
        .attr("height", h - this.yDataMargin);
    },

    colorChanged: function() {
      if (typeof(this.dots) === 'undefined') {
        return;
      }
      this.dots
        .attr("color", this.color);
    },

    verifyMax: function() {
      var self = this;
      var xMax = this.values.reduce(function(xMax, v) {
        return Math.max(v.x, xMax);
      }, this.xMax);
      if (xMax != this.xMax) {
        this.xMax = xMax;
        this.calculateSizes(this.element);
      }
    },

    valuesChanged: function() {
      if (typeof(this.plot) === 'undefined' || typeof(this.values) === 'undefined') {
        return;
      }
      var self = this;
      this.verifyMax();
      this.labels = this.values.map(this.yValue);
      this.yScale.domain(this.labels);
      this.yAxisG.call(this.yAxis);

      this.plot.selectAll('.bargraph-bar').remove();

      this.bars = this.plot.selectAll('.bargraph-bar')
        .data(this.values)
        .enter()
        .append("rect")
        .attr("height", self.yScale.rangeBand)
        .attr("width", self.xWidth)
        .attr("x", 0)
        .attr("y", self.yMap)
        .attr('class', 'bargraph-bar')
        .style("fill", this.color);

      this.plot.selectAll('.bargraph-txt').remove();
      this.barsLabel = this.plot.selectAll('.bargraph-txt')
        .data(this.values)
        .enter()
        .append('text')
        .attr('class', 'bargraph-txt')
        .attr('x', function(d) {
          return self.xWidth(d) + 10;
        })
        .attr('y', function(d) {
          return self.yMap(d) + (self.yScale.rangeBand(d) / 2);
        })
        .text(function(d) {
          return '' + d.x;
        });
    },

    update: function() {
      var self = this;
      this.verifyMax();
      this.bars
        .transition()
        .duration(1000)
        .attr("height", self.yScale.rangeBand)
        .attr("width", self.xWidth)
        .attr("y", self.yMap);

      this.barsLabel
        .transition()
        .duration(1000)
        .attr('x', function(d) {
          return self.xWidth(d) + 10;
        })
        .attr('y', function(d) {
          return self.yMap(d) + (self.yScale.rangeBand(d) / 2);
        })
        .text(function(d) {
          return '' + d.x;
        });

    }
  }

})(window, window.angular);
