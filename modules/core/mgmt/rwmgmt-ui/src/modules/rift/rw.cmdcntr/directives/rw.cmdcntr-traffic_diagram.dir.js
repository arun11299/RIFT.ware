module.exports = (function(window, angular){
  "use strict";


  angular.module('cmdcntr')
    .directive('trafficDiagram', function() {

      /**
       * <traffic-diagram> -
       */
      function controller($scope, $rootScope, $element, radio, $compile) {
        var self = this;
        var appChannel = radio.channel('appChannel');
        self.appChannel = appChannel;
        this.compile = function(elem) {
          return $compile(elem)($scope)[0];
        };

        $scope.$watch(function() {
            return self.services;
          },
          function(newVal, oldVal) {
            if (newVal) {
              self.servicesChanged();
            }
          }
        );
        rw.BaseController.call(this);
      };

      angular.extend(controller.prototype, {
        servicesChanged: function() {
          var self = this;
          var gxDiagram = d3.select('#traffic-diagram');
          this.layout = new rw.TopologyLayout();
          this.layout.connectorStore = {};
          this.layout.dim.width = 300;
          this.layout.dim.hIface = 285;
          this.layout.dim.yMargin = 70;
          var widgetDim = {
            trafgen : { h : 0,  tag : 'trafgen-summary' },
            loadbal : { h : 50,  tag : 'loadbal-summary' },
            trafsimclient : { h : 0,  tag : 'trafsim-summary' },
            trafsimserver : { h : 0,  tag : 'trafsim-summary' },
            trafsink : { h : 0,  tag : 'trafgen-summary' },
            ltemmesim : { h : 0,  tag : 'lte-summary' },
            ltegwsim : { h : 0,  tag : 'lte-summary' },
            slbalancer : { h : 0,  tag : 'slbalancer-summary' },
            iot_army : { h : 0,  tag : 'trafgen-summary' },
            iot_server: { h : 0,  tag : 'trafgen-summary' },
            premise_gw: { h : 0, tag: 'slbalancer-summary'},
            cag: {h: 0, tag: 'slbalancer-summary'}
          };

          var gxServices = gxDiagram.selectAll("g.service")
            .data(function() {
              for (var i = 0; i < self.services.length; i++) {
                var h = widgetDim[self.services[i].type].h;
                var nStats = 1;
                if (self.services[i].connector) {
                  nStats = Math.max(1, self.services[i].connector.length);
                }
                var bodyHeight = (nStats * self.layout.dim.hIface) + h + 15;

                self.layout.layoutTitledPanel(self.services[i],
                  self.layout.colIndex(self.services[i], self.services), bodyHeight);
              }
              return self.services;
            })
            .enter()
            .append("g")
            .classed('service', true);

          self.layout.appendTitledPanel(gxServices);
          self.layout.appendTitle(gxServices, function(service) {
            self.serviceSelected(service);
          });

          var gxIfaces = self.layout.appendIfaces(gxServices,
            function(service) {
              var h = widgetDim[service.type].h;
              service.yStats = service.yBody + h;
              return h;
            },
            function(iface, service, h) {
              iface.x = service.x;
              iface.y = service.yBody + h;
              iface.w = self.layout.dim.width;
              iface.h = self.layout.dim.hIface;
              return h + iface.h;
            }
          );

          self.layout.appendConnectorHandles(gxIfaces);
          self.layout.appendConnectors(gxDiagram);

          var gxMetrics = d3.select('#traffic-metrics');

          var gxServices = gxMetrics.selectAll('.metric')
            .data(self.services)
            .enter()
            .append(function(service, i) {
              var elementType = widgetDim[service.type].tag;
              var widget = document.createElement(elementType);
              widget.className = 'metric'
              widget.style.width = (service.w - 25) + "px";
              widget.style.height = service.h + "px";
              widget.style.top = service.yBody + "px";
              widget.style.left = (service.x + 2) + "px";
              widget.setAttribute('service', "trafficDiagram.services[" + i + "]")
              var angWidget = self.compile(widget);
              return angWidget;
            });

          self.serviceSelected(self.services[0]);
        },

        serviceSelected : function(service) {
          var self = this;
          if (this.selectBox) {
            this.selectBox.remove();
          }

          this.selectBox = document.createElementNS("http://www.w3.org/2000/svg", "rect");
          this.selectBox.setAttribute("class", "selection");
          var w = service.w + 8;
          var h = service.h + 8;
          this.selectBox.setAttribute("width", w);
          this.selectBox.setAttribute("height", h);
          var dashArray = this.layout.buildSelectBox(w, h, 64);
          this.selectBox.setAttribute("stroke-dasharray", dashArray);
          document.getElementById('traffic-diagram').appendChild(this.selectBox);
          this.selectBox.setAttribute("x", service.x - 4);
          this.selectBox.setAttribute("y", service.y - 4);

          this.appChannel.trigger('select-service', service, self);
        }
      });

      return {
        restrict: 'AE',
        templateUrl: '/modules/views/rw.cmdcntr-traffic_diagram.tmpl.html',
        controller: controller,
        controllerAs: "trafficDiagram",
        bindToController : true,
        replace: true,
        scope : {
          services : '='
        }
      };
    });

})(window, window.angular);
