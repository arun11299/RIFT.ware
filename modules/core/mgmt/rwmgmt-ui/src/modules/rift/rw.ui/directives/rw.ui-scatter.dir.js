// Q: How do i test $scope?
// Q: Is there a useful way to note js and css dependencies
// Q: How do i jsdocument this?
(function(window, angular) {

  angular.module('uiModule')
    .directive('rwScatter', function() {
      var controller = function($scope, $element) {
        $scope.bulletColor = $scope.bulletColor || 'rgba(73, 189, 238, 1)';
        $scope.margin = {
          top: 20,
          right: 20,
          bottom: 20,
          left: 35
        };
        $scope.range = {
          xMin : 0,
          xMax : 100,
          yMin : 0,
          yMax : 100
        };
        $scope.TooltipOutline = function(d3Item, orientation) {
          this.line = d3Item
            .append("path")
            .attr("stroke-dasharray", "5,5")
            .attr("class", "scatter-tooltipGuide");

          this.txt = d3Item
            .append("text")
            .attr("class", "scatter-tooltipGuideLabel");
          this.orientation = orientation;
          //console.log(orientation);
          return this;
        };

        $scope.TooltipOutline.prototype = {
          horizontal: 1,

          vertical: 2,

          // how far off x or y axis to show label
          labelOffset: 4,

          // when value is so small, no room to draw
          // line but actual value is easily seen as it's
          // so close to axis
          threshold: 5,

          show: function (d, x, y, height) {
            var path = ['M', x, y, 'L'];

            var shouldShow;
            if (this.orientation === "horizontal") {
              Array.prototype.push.call(path, 0, y);
              this.txt.text('' + Math.round(d.y));
              this.txt
                .attr("x", 0 + this.labelOffset)
                .attr("y", y - this.labelOffset);
              shouldShow = (d.x > this.threshold);
            } else {
              Array.prototype.push.call(path, x, height);
              this.txt.text('' + Math.round(d.x));
              this.txt
                .attr("x", x + this.labelOffset)
                .attr("y", height - this.labelOffset);
              shouldShow = (d.y > this.threshold);
            }
            if (shouldShow) {
              this.line
                .attr('d', path.join(' '))
                .transition()
                .duration(200)
                .style("opacity", .9);
              this.txt
                .transition()
                .duration(200)
                .style("opacity", .9);
            }
          },
          hide: function () {
            this.line.transition()
              .duration(500)
              .style("opacity", 0);
            this.txt
              .transition()
              .duration(500)
              .style("opacity", 0);
          }
        };

        $scope.sizeToFit = function() {
          var self = $scope;
          $scope.svg
            .attr("width", $scope.plotWidth + self.margin.right + self.margin.left)
            .attr("height", $scope.plotHeight + self.margin.top + self.margin.bottom);

          $scope.plot
            .attr("transform", "translate(" + self.margin.left + "," + self.margin.top + ")");

          $scope.xAxisG
            .attr("transform", "translate(0," + $scope.plotHeight + ")")
            .call($scope.xAxis);

          $scope.xAxisText.attr("x", $scope.plotWidth);
          $scope.yAxisG.call($scope.yAxis);
        },

        $scope.$watch('bulletColor', function() {
          if (typeof($scope.dots) === 'undefined') {
            return;
          }
          $scope.dots
            .attr("color", $scope.bulletColor);
        });

        $scope.valuesChanged = function() {
          if (typeof($scope.plot) === 'undefined' || typeof($scope.values) === 'undefined') {
            return;
          }
          var self = $scope;
          $scope.dotsContainer.selectAll('.scatter-dot').remove();
          $scope.dots = $scope.dotsContainer.selectAll('.dot')
            .data($scope.values)
            .enter()
            .append("circle")
            .attr("r", function(d) { return d.size;})
            .attr("cx", $scope.xMap)
            .attr("cy", $scope.yMap)
            .attr('class', 'scatter-dot')
            .style("fill", $scope.bulletColor)
            .on("mouseover", self.showToolTip.bind(self))
            .on("mouseout", self.hideTooltip.bind(self));
        };
        $scope.$watch('values', $scope.valuesChanged.bind($scope));

        $scope.showToolTip = function(d) {
          // moves circle to top in case it's behind others
          var node = d3.event.srcElement;
          var pagePointer = {
            x : d3.event.offsetX - 15,
            y : d3.event.offsetY - 40
          };

          var canvas_width = d3.event.relatedTarget.width.baseVal.value;
          var node_cx = node.cx.baseVal.value;
          console.log(canvas_width);
          console.log(node_cx);

          if (canvas_width - node_cx < 120) {
            pagePointer.x -= (canvas_width - node_cx) + 30;
          }

          if (typeof(node) === 'undefined') {
            // Firefox
            node = d3.event.currentTarget;
            pagePointer = {
              x : d3.event.pageX - 10,
              y : d3.event.pageY - 50
            };
          }
          node.parentNode.appendChild(node);
          $scope.tooltip.transition()
            .duration(200)
            .style("opacity", .9);
          $scope.tooltip.html(d.label)
            .style("left", pagePointer.x + "px")
            .style("top", pagePointer.y + "px");

          var x = $scope.xMap(d);
          var y = $scope.yMap(d);
          $scope.xTooltipOutline.show(d, x, y, $scope.plotHeight);
          $scope.yTooltipOutline.show(d, x, y, $scope.plotHeight);

          // IE doesn't support mouseout if the dom element is modified so
          // we have to close it on a timer
          //   http://stackoverflow.com/questions/3686132/move-active-element-loses-mouseout-event-in-internet-explorer
          if (platform.name == 'IE') {
            setTimeout($scope.hideTooltip.bind($scope), 3000);
          }
        };

        $scope.hideTooltip = function() {
          $scope.tooltip.transition()
            .duration(500)
            .style("opacity", 0);
          $scope.yTooltipOutline.hide();
          $scope.xTooltipOutline.hide();
        };

        $scope.update = function() {
          $scope.dots
            .transition()
            .duration(1000)
            .attr("r", function(d) { return d.size;})
            .attr("cx", $scope.xMap)
            .attr("cy", $scope.yMap);
        };
        $scope.$on("scatter-values-update", $scope.update.bind($scope));

        $scope.calculateSizes = function() {
          var self = $scope;
          var dim = $element[0].getBoundingClientRect();
          $scope.height = Math.floor(dim.height);
          $scope.width = Math.floor(dim.width);

          $scope.plotWidth = $scope.width - $scope.margin.left - $scope.margin.right;
          $scope.plotHeight = $scope.height - $scope.margin.top - $scope.margin.bottom;

          $scope.xValue = function(d) { return d.x; };
          $scope.xScale = d3.scale.linear().range([0, $scope.plotWidth]);
          $scope.xAxis = d3.svg.axis().scale($scope.xScale).orient("bottom");
          $scope.xMap = function(d) {return self.xScale(self.xValue(d)); };
          $scope.xScale.domain([$scope.range.xMin, $scope.range.xMax]);

          $scope.yValue = function(d) { return d.y; };
          $scope.yScale = d3.scale.linear().range([$scope.plotHeight, 0]);
          $scope.yAxis = d3.svg.axis().scale($scope.yScale).orient("left");
          $scope.yMap = function(d) { return self.yScale(self.yValue(d)); };
          $scope.yScale.domain([$scope.range.yMin, $scope.range.yMax]);
        };

        $scope.resize = function() {
          $scope.calculateSizes();
          $scope.sizeToFit();
        };


        angular.element(document).ready( function() {
            var self = $scope;
            $scope.calculateSizes();

            // add the graph canvas to the body of the webpage
            $scope.svg = d3.select($element[0].children[0]);
            $scope.plot = $scope.svg.append("g");

            // x-axis
            $scope.xAxisG = $scope.plot.append("g")
              .attr("class", "scatter-x scatter-axis");

            $scope.xAxisText = $scope.xAxisG
              .append("text")
              .attr("class", "scatter-label")
              .attr("y", -6)
              .style("text-anchor", "end")
              .text($scope.exsAxisLabel);

            // y-axis
            $scope.yAxisG = $scope.plot
              .append("g")
              .attr("class", "scatter-y scatter-axis");

            $scope.yAxisText = $scope.yAxisG
              .call($scope.yAxis)
              .append("text")
              .attr("class", "scatter-label")
              .attr("transform", "rotate(-90)")
              .attr("y", 6)
              .attr("dy", ".71em")
              .style("text-anchor", "end")
              .text($scope.yAxisLabel);
            $scope.dotsContainer = $scope.plot.append("g");
            // add the tooltip area to the webpage
            $scope.tooltip = d3.select($element[0])
              .append("div")
              .attr("class", "scatter-tooltip")
              .style("opacity", 0);

            $scope.yTooltipOutline = new $scope.TooltipOutline($scope.plot, "horizontal");
            $scope.xTooltipOutline = new $scope.TooltipOutline($scope.plot, "vertical");

            $scope.sizeToFit();
            $scope.valuesChanged();

            window.addEventListener("resize", _.debounce(self.resize.bind(self), 50));
          }
          )
      };

      return {
        restrict : 'E',
        template: '<div class="scatter-plotContainer"><svg></svg></div>',
        controller : controller,
        replace: true,
        scope: {
          min : '@',
          max : '@',
          values : '=',
          marker: '@',
          bulletColor: '@',
          //
          // ANGULAR BUG - If I named this 'xAxisLabel' it would not work, something
          // is eating the definition on the $scope
          //  xAxisLabel : '@
          //
          exsAxisLabel: '@',
          yAxisLabel: '@'
        }
      };
    });

})(window, window.angular);
