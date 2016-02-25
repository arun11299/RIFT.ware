module.exports = (function(window, angular) {
  "use strict";
  angular.module('cmdcntr')
    .directive('networkDiagram', ['radio', networkDiagram]);

  function networkDiagram(radio) {
    var controller = function($scope, $element, $rootScope, $timeout) {
      var appChannel = radio.channel('appChannel');
      var links;

      $scope.run = false;
      $scope.firstIface = null;

      $scope.servicesChanged = function() {
        if (typeof($scope.services) === 'undefined' || $scope.services == null) {
          return;
        }

        var self = this;
        self.services = $scope.services;
        var serviceLayout = {
          curveRadius : 4
        };
        var ifaceLayout = {
          h: 35,
          margin : 10
        };
        var handleLayout = {
          h: 30,
          w: 80
        };
        var x = function(o) {
          return o.x;
        };
        var y = function(o) {
          return o.y;
        };
        var w = function(o) {
          return o.w;
        };
        var h = function(o) {
          return o.h;
        };
        var icon = "M328.047,50l-60.643,145.755l144.312-16.243l-33.971,39.608l-215.225,25.37L328.047,50z "
         + "M348.578,259.808l-214.541,25.288l-33.754,40.206l141.002-15.558L171.775,462L348.578,259.808z";

        var gxDiagram = d3.select('#diagram');
        var layout = new rw.TopologyLayout();
        this.layout = layout;
        this.layout.connectorStore = {};
        layout.columns = self.services.length;
        layout.dim.xMargin = [20, 550, 600, 650];

        var gxServices = gxDiagram.selectAll("g.service")
          .data(function() {
            var countPorts = function(n, connector) {
              return 'interface' in connector ? n + connector.interface.length : 0;
            };
            for (var i = 0; i < self.services.length; i++) {
              var service = self.services[i];
              var n;
              if (service.connector) {
                n = service.connector.reduce(countPorts, 0);
              } else {
                n = 0;
              }
              var bodyHeight = (n * ifaceLayout.h) + ((n + 1) * ifaceLayout.margin);
              service.numberPorts = n;
              layout.layoutTitledPanel(service, layout.colIndex(service, self.services), bodyHeight);
            }
            return self.services;
          })
          .enter()
          .append("g")
          .classed('service', true);

        layout.appendTitledPanel(gxServices);
        layout.appendTitle(gxServices);

        var gxIfaces = layout.appendIfaces(gxServices,
          function(service) {
            return 0;
          },
          function(connector, service, h) {
            connector.x = service.x;
            connector.y = service.yBody + h;
            connector.w = layout.dim.width;
            var n = 'interface' in connector ? connector.interface.length : 0;
            connector.h = (n * (ifaceLayout.h + ifaceLayout.margin));
            return h + connector.h;
          }
        );

        layout.appendConnectorHandles(gxIfaces);
        links = layout.appendConnectors(gxDiagram);

        gxDiagram.selectAll("circle.context")
          .data(function() { return links; })
          .enter()
          .append("circle")
          .classed('context', true)
          .attr('cx', function(l) { return l.xStart + l.xMiddle;})
          .attr('cy', function(l) { return l.yStart + l.yMiddle;})
          .attr('r', 45);

        gxDiagram.selectAll("text.context")
          .data(function() { return links; })
          .enter()
          .append("text")
          .text(function(l) {
            return l.source['@context'];
           })
          .classed('context', true)
          .attr('text-anchor', 'middle')
          .attr('x', function(l) { return l.xStart + l.xMiddle;})
          .attr('y', function(l) { return l.yStart + l.yMiddle;});

        var gxPorts = gxIfaces.selectAll("svg.iface")
          .data(function(connector) {
            if (!('interface' in connector)) {
              return [];
            }
            for (var i = 0; i < connector.interface.length; i++) {
              var iface = connector.interface[i];
              iface.x = connector.x + ifaceLayout.margin;
              iface.y = connector.y + ifaceLayout.margin +
                (i * (ifaceLayout.h + ifaceLayout.margin));
              iface.h = ifaceLayout.h;
              iface.w = connector.w - (2 * ifaceLayout.margin);
              // provide reference back to group for useful event firing
              iface.parent = connector;
              if(i == 0 && !$scope.run){
                $scope.run = true;
                $scope.firstIface = iface;
              }
            };
            return connector.interface;
          })
          .enter()
          .append("svg")
          .classed('iface', true)
          .attr("x", x)
          .attr("y", y)
          .attr("width", w)
          .attr("height", h)
          .on('click', function(iface,index) {
            $scope.selectInterface.call(self, iface);
          });

        gxPorts
          .append("rect")
          .classed('iface', true)
          .attr("width", w)
          .attr("height", h);

        gxPorts
          .append("text")
          .classed('label', true)
          .text(function(iface) {
            return iface.name;
          })
          .attr("x", function(iface) {
            return 40;
          })
          .attr("y", function(iface) {
            return 24;
          });

        var gxIcon = gxPorts
          .append("svg")
          .classed('icon', true)
          .attr('width', h)
          .attr('height', h)
          .attr('x', 0)
          .attr('y', 0)
          .attr('viewBox', '0 0 512 512');

        gxIcon
          .append("rect")
          .attr('width', 512)
          .attr('height', 512);

        gxIcon
          .append("path")
          .attr('fill', 'white')
          .attr('d', icon);


        $timeout(function() {
          $scope.selectInterface($scope.firstIface);
        });

      };

      $scope.selectInterface = function(iface) {
        if (typeof(iface) == 'undefined' || iface == null) {
          return;
        }
        if (!this.selectBox) {
          this.selectBox = document.createElementNS("http://www.w3.org/2000/svg", "rect");
          this.selectBox.setAttribute("class", "selection");
          var w = iface.w + 8;
          var h = iface.h + 8;
          this.selectBox.setAttribute("width", w);
          this.selectBox.setAttribute("height", h);
          var dashArray = this.layout.buildSelectBox(w, h, 20);
          this.selectBox.setAttribute("stroke-dasharray", dashArray);
          $('#diagram').append(this.selectBox);
        }

        this.selectBox.setAttribute("x", iface.x - 4);
        this.selectBox.setAttribute("y", iface.y - 4);

        appChannel.trigger("select-interface", {
          iface : iface,
          service : iface.parent.parent
        });
      };

      // watch services for changes so you can build the diagram once it changes
      $scope.$watch('services', $scope.servicesChanged.bind($scope));
      rw.BaseController.call(this);
    };

    return {
      restrict: 'AE',
      templateUrl: '/modules/views/rw.cmdcntr-network_diagram.tmpl.html',
      controller: controller,
      scope: {
        services: '='
      },
      replace: true
    }
  };
})(window, window.angular);
