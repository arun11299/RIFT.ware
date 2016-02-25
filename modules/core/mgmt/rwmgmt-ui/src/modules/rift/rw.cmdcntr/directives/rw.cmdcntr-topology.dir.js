module.exports = (function(window, angular) {
  "use strict";
  angular.module('cmdcntr')
    .directive('topology', ['radio', topology]);

  function topology(radio) {
    var controller = function($scope, vnfFactory, radio, $timeout) {
    	var self = this;
    	var appChannel = radio.channel('appChannel');

    	$scope.services = [];
    	vnfFactory.attached(true).done(function() {

    		$scope.services = vnfFactory.services;

		    var gxDiagram = d3.select('#diagram');
		    var layout = new rw.TopologyLayout();
		    layout.connectorStore = {};
		    layout.dim.yMargin0 = 40;
		    layout.dim.yMargin = 60;
		    layout.dim.xMargin = [30, 405, 440, 475];
		    layout.dim.width = 260;
		    var vmLayout = {
		      h: 35,
		      margin : 10
		    };
		    var asLayout = {
		      w: 35,
		      wLabel: 100
		    };
		    var services = $scope.services;
		    // copied from from rw-iconset.html
		    var icon = "M462,270.2H50v103.3h412V270.2z M77.4,344.5l18.1-45.3h21.9l-18.1,45.3H77.4z M112.9,344.5l18.1-45.3h21.9l-18.1,45.3H112.9z " +
		      "M148.3,344.5l18.1-45.3h21.9l-18.1,45.3H148.3z M183.8,344.5l18.1-45.3h21.9l-18.1,45.3H183.8z M241.2,344.5h-21.9l18.1-45.3h21.9L241.2,344.5z " +
		      "M413.1,337c-8.4,0-15.1-6.8-15.1-15.1s6.8-15.1,15.1-15.1c8.4,0,15.1,6.8,15.1,15.1S421.4,337,413.1,337z " +
		      "M439.3,243.1H73.6l72.1-95.2h220.6L439.3,243.1z";
		    var gxServices = gxDiagram.selectAll("g.service")
		      .data(function() {
		        _.each(services, function(service) {
		          service.vm = service.vm || [];
		          var n = service.vm.length;
		          var bodyHeight = Math.max(130, (n * vmLayout.h) + ((n + 1) * vmLayout.margin));
		          service.nScalars = service.connector ? service.connector.length : 0;
		          var bodyWidth = layout.dim.width + (Math.min(2, service.nScalars) * asLayout.w);
		          layout.layoutTitledPanel(service, layout.colIndex(service, services), bodyHeight, bodyWidth);
		        });
		        return services;
		      })
		      .enter()
		      .append("g")
		      .classed('service', true);

		    // fabric
		    gxServices.append("rect")
		      .classed('fabric', true)
		      .attr('fill', 'url(#fabric)')
		      .attr("x", function(d) {return d.x - 20;})
		      .attr("y", function(d) {return d.y - 20;})
		      .attr("width", function(d) {return d.w + 40;})
		      .attr("height", function(d) {return d.h + 40;})
		      .attr("rx", layout.dim.curveRadius)
		      .attr("ry", layout.dim.curveRadius);

		    gxServices.append("text")
		      .classed('fabric', true)
		      .attr("x", function(d) {return d.x - 18;})
		      .attr("y", function(d) {return d.y - 5;})
		      .text('FABRIC');

		    layout.appendTitledPanel(gxServices);
		    layout.appendTitle(gxServices);

		    layout.connectors = {};
		    var gxAutoScalars = gxServices.selectAll("svg.as")
		      .data(function(service) {
		        if (!("connector" in service)) {
		          return []
		        }
		        var connectors = [];
		        service.asRight = service.asLeft = false;
		        for (var i = 0; i < service.connector.length; i++) {
		          var connector = service.connector[i];
		          // provide reference from iface to service so onclick can send service object in event details
		          connector.parent = service;
		          // connector.faceRight = (undefined !== connector.destination && connector.destination.length > 0);
		          connector.faceRight = layout.connectorHandleFaceRight.call(layout, connector);
		          if (connector.faceRight) {
		            connector.xConnector = service.x + service.w - 10;
		            connector.x = connector.xConnector - asLayout.w;
		          } else {
		            connector.x = service.x + 10;
		            connector.xConnector = connector.x;
		          }
		          connector.h = (service.h - layout.dim.hTitle) - 20;
		          connector.y = service.y + layout.dim.hTitle + vmLayout.margin;
		          connector.yConnector = connector.y + (connector.h / 2);
		          connector.w = asLayout.w;
		          service.asRight = service.asRight || connector.faceRight;
		          service.asLeft = service.asLeft || (! connector.faceRight);
		          layout.connectors[connector['@id']] = connector;
		          connectors.push(connector);
		        }
		        return connectors;
		      })
		      .enter()
		      .append("svg")
		      .classed('as', true)
		      .attr("x", layout.x)
		      .attr("y", layout.y)
		      .attr("width", layout.w)
		      .attr("height", layout.h);

		    gxAutoScalars
		      .append("rect")
		      .classed('as', true)
		      .attr("width", layout.w)
		      .attr("height", layout.h);

		    gxAutoScalars
		      .append("text")
		      .classed('as', true)
		      .attr('width', layout.h)
		      .attr('text-anchor', 'middle')
		      .attr('x', function(d) {
		        return (d.w / 2);
		      })
		      .attr('y', function(d) {
		        return (d.h / 2) + 6;
		      })
		      .attr('transform', function(d) {
		        return 'rotate(270, ' + (d.w / 2) + ',' +  (d.h / 2) + ')';
		      })
		      .text('Autoscalar');

		    layout.appendConnectors(gxDiagram);

		    var gxVms = gxServices.selectAll("svg.vm")
		      .data(function(service) {
		        _.each(service.vm, function(vm, i) {
		          var asr = service.asRight ? asLayout.w + 5 : 0;
		          var asl = service.asLeft ? asLayout.w + 5 : 0;
		          vm.x = service.x + asl + vmLayout.margin;
		          vm.y = service.y + layout.dim.hTitle + vmLayout.margin +
		                  (i * (vmLayout.h + vmLayout.margin));
		          vm.h = vmLayout.h;
		          vm.w = service.w - ((2 * vmLayout.margin) + asr + asl);
		        });
		        return service.vm;
		      })
		      .enter()
		      .append("svg")
		      .classed('iface', true)
		      .attr("x", layout.x)
		      .attr("y", layout.y)
		      .attr("width", layout.w)
		      .attr("height", layout.h);

		    gxVms
		      .append("rect")
		      .classed('iface', true)
		      .attr("width", layout.w)
		      .attr("height", layout.h);

		    gxVms
		      .append("text")
		      .classed('label', true)
		      .text(function(vm) {
		        return vm.instance_name;
		      })
		      .attr("x", function(vm) {
		        return 40;
		      })
		      .attr("y", function(vm) {
		        return 24;
		      });

		    var gxIcon = gxVms
		      .append("svg")
		      .attr('width', function(d) { return d.h - 4;})
		      .attr('height', function(d) { return d.h - 4;})
		      .attr('x', 2)
		      .attr('y', 2)
		      .attr('viewBox', '0 0 512 512');

		    gxIcon
		      .append("rect")
		      .classed('vmicon', true)
		      .attr('width', 512)
		      .attr('height', 512);

		    gxIcon
		      .append("path")
		      .attr('fill', 'white')
		      .attr('d', icon);
    	});

		$scope.$on('$stateChangeStart', function() {
            vnfFactory.detached();
        });
    };

    return {
    	restrict: 'AE',
    	templateUrl: '/modules/views/rw.cmdcntr-topology.tmpl.html',
    	scope: {
    		// services: '='
    	},
    	controller: controller,
    	replace: true
    };
  };
})(window, window.angular);
