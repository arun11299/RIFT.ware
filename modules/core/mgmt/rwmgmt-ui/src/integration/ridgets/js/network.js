
function TopologyLayout() {
  //  trafgen, load bal, other
  this.grid = [];
  this.dim = {
    yMargin0 : 20,
    xMargin : [30, 500, 500, 650],
    yMargin : 10,
    hTitle: 35,
    width : 250,
    curveRadius : 4,
    hConnector: 30,
    wConnector: 80
  };
}

TopologyLayout.prototype.x = function(o) {
  return o.x;
};

TopologyLayout.prototype.y = function(o) {
  return o.y;
};

TopologyLayout.prototype.w = function(o) {
  return o.w;
};

TopologyLayout.prototype.h = function(o) {
  return o.h;
};

TopologyLayout.prototype.layoutTitledPanel = function(o, colNdx, bodyHeight, bodyWidth) {
  for (var i = this.grid.length; i <= colNdx; i++) {
    this.grid.push([]);
  }
  var col = this.grid[colNdx];
  if (col.length === 0) {
    o.y = this.dim.yMargin0;
  } else {
    o.y = col[col.length - 1].y + col[col.length - 1].h + this.dim.yMargin;
  }
  var w = bodyWidth ? bodyWidth : this.dim.width;

  var xMargin = 0;
  console.log(this.dim.xMargin, colNdx)
  for (i = 0; i <= colNdx; i++) {
    xMargin += this.dim.xMargin[i];
  }
  o.x = xMargin;
  o.w = w;
  o.h = this.dim.hTitle + bodyHeight;
  o.yBody = o.y + this.dim.hTitle;
  col.push(o);
};

TopologyLayout.prototype.colIndex = function(service, services) {
  // RIFT-4304
  if (services.length == 1 || !('connector' in service)) {
    return 0;
  }

  // return 1 more column then the one we're connected to
  for (var i = 0; i < services.length; i++) {
    var destinations = jsonPath.eval(services[i], 'connector[*].destination[*]');
    for (var j = 0; j < destinations.length; j++) {
      for (var k  = 0; k < service.connector.length; k++) {
        if (service.connector[k]['@id'] == destinations[j]) {
          var neighborColIndex = services[i].colIndex;
          if (typeof(neighborColIndex) === 'undefined') {
            neighborColIndex = this.colIndex(services[i], services);
          }
          service.colIndex = neighborColIndex + 1;
          return service.colIndex;
        }
      }
    }
  }

  service.colIndex = 0;
  return service.colIndex;
};

TopologyLayout.prototype.appendTitledPanel = function(gxServices) {
  var s = gxServices
    .append("rect")
    .classed('service', true)
    .attr("x", this.x)
    .attr("y", this.y)
    .attr("width", this.w)
    .attr("height", this.h)
    .attr("rx", this.dim.curveRadius)
    .attr("ry", this.dim.curveRadius);

  return s;
};

TopologyLayout.prototype.appendTitle = function(gxServices, onClick) {
    var gxTitle = gxServices
      .append("svg")
      .attr('width', this.w)
      .attr('height', this.dim.hTitle)
      .attr('x', this.x)
      .attr('y', function(service) {
        return service.y;
      });

    gxTitle
      .append("rect")
      .attr('width', this.w)
      .attr('height', this.dim.hTitle)
      .classed("title-background", true);

    gxTitle
      .append("text")
      .attr("x", 10)
      .attr("y", this.dim.hTitle - 10)
      .classed("title", true)
      .text(function(service) {
        return rw.ui.l10n.vnf[service.type];
      });

    var self = this;
    gxTitle
      .append("path")
      .classed('hr', true)
      .attr("d", function(d) {
        return ['M 0', self.dim.hTitle, 'l', d.w, '0'].join(' ');
      });

    if (onClick) {
      gxTitle.on("click", onClick);
    }

    return gxTitle;
};

TopologyLayout.prototype.appendIfaces = function(gxServices, layoutService,
    layoutIface) {
  var self = this;
  this.connectors = {};
  var gxIfaces = gxServices.selectAll("g")
    .data(function(service) {
      var layoutState = layoutService(service);
      if (!("connector" in service)) {
        return []
      }
      for (var i = 0; i < service.connector.length; i++) {
        var connector = service.connector[i];
        self.connectors[connector['@id']] = connector;

        // provide reference from iface to service so onclick can send service object in event details
        connector.parent = service;

        layoutState = layoutIface(connector, service, layoutState);
      }
      return service.connector;
    })
    .enter()
    .append("g");

  return gxIfaces;
};

TopologyLayout.prototype.connectorHandleFaceRight = function(iface) {
  // RIFT-4304
  console.log(iface)
  if (iface.parent.x <= this.dim.xMargin[0]) {
    return true;
  }
  // Convention:
  //   Clients to left, server to right
  return (undefined !== iface.destination && iface.destination.length > 0);
}

TopologyLayout.prototype.appendConnectorHandles = function(gxIfaces) {
  var self = this;
  var gxConnectorHandle = gxIfaces
    .append("g")
    .classed("handle", true);

  gxConnectorHandle
    .append("path")
    .attr("d", function(iface) {
      iface.faceRight = self.connectorHandleFaceRight.call(self, iface);
      iface.xSelector = iface.x + (iface.faceRight ? iface.w - 8 : 8);
      iface.xHandleTip = iface.xSelector + (iface.faceRight ? 4 : -4);
      var yConnectorStart = iface.y + 5;
      var hConnector = iface.h - 8;
      iface.yConnector = iface.y + (iface.h / 2);
      var path = [
        'M', iface.xSelector, yConnectorStart,
        'L', iface.xSelector, yConnectorStart + hConnector,
        'M', iface.xSelector, iface.yConnector,
        'L', iface.xHandleTip, iface.yConnector
      ].join(' ');
      return path;
    });

  var gxBubble = gxConnectorHandle
    .append("svg")
    .attr("x", function(iface) {
      var xBubble = iface.xHandleTip + (iface.faceRight ? 0 : - self.dim.wConnector);
      iface.xConnector = xBubble + (iface.faceRight ? self.dim.wConnector : 0);
      return xBubble;
    })
    .attr("y", function(iface) {
      return iface.yConnector - (self.dim.hConnector / 2);
    })
    .attr("w", self.dim.wConnector + 2)
    .attr("h", self.dim.hConnector + 2);

  gxBubble
    .append("rect")
    .attr("rx", 14)
    .attr("ry", 14)
    // rect gets clipped otherwise
    .attr("width", self.dim.wConnector - 4)
    .attr("height", self.dim.hConnector)
    .attr("x", function(iface) {
      return 2;
    })
    .attr("y", function(iface) {
      return 2;
    });

  var gxBubbleText = gxBubble.append("text");

  gxBubbleText
    .attr("class", "handle-label")
    .attr("text-anchor", "middle");

  gxBubbleText
    .append("tspan")
    .attr("alignment-baseline", "hanging")
    .text(function(connector) {
      if (!('interface' in connector)) {
        return '';
      }
      var speed = 0;
      connector.interface.forEach(function(iface) {
        speed += parseInt(iface.port[0].speed);
      });
      return (speed / 1000) + ' Gbps';
    });

  // HACK: IE and Firefox seem to render text lower, Safari and Chrome same
  var browserVendorFudge = 7;
  if (platform.name == 'IE' || platform.name == 'Firefox') {
    browserVendorFudge = 22;
  }

  gxBubbleText
    .attr("x", function(iface) {
      return self.dim.wConnector / 2;
    })
    .attr("y", function(iface) {
      return browserVendorFudge;
    });
};


TopologyLayout.prototype.buildSelectBox = function(w, h, dashLen) {
  // do a little algebra to give corner-dash-gap-...corner effect
  var vCount = Math.floor(w / (2 * dashLen));
  var vDashLen = w / (2 * vCount);
  var vGapLen = vDashLen;
  var hCount = Math.floor(h / (2 * dashLen));
  var hDashLen = h / (2 * hCount);
  var hGapLen = hDashLen;
  var cornerLen = (vDashLen + hDashLen) / 2;
  var i;

  // start at top left corner and go counter clockwise adding lengths
  // that with be [ line, gap, line, gap.... ]

  // TOP
  var dashArray = [ vDashLen / 2 ];
  for (i = 0; i < vCount - 1; i++) {
    dashArray.push(vGapLen);
    dashArray.push(vDashLen);
  }
  dashArray.push(vGapLen);
  dashArray.push(cornerLen);
  // RIGHT
  for (i = 0; i < hCount - 1; i++) {
    dashArray.push(hGapLen);
    dashArray.push(hDashLen);
  }
  dashArray.push(hGapLen);
  dashArray.push(cornerLen);
  // BOTTOM
  for (i = 0; i < vCount - 1; i++) {
    dashArray.push(vGapLen);
    dashArray.push(vDashLen);
  }
  dashArray.push(vGapLen);
  dashArray.push(cornerLen);
  // LEFT
  for (i = 0; i < hCount - 1; i++) {
    dashArray.push(hGapLen);
    dashArray.push(hDashLen);
  }
  dashArray.push(hGapLen);
  dashArray.push(hDashLen / 2);

  return dashArray.join(', ');
};

TopologyLayout.prototype.appendConnectors = function(gxDiagram) {
  var self = this;
  var links = d3.values(this.connectors).reduce(function(links, connector) {
    if (connector.destination) {
      for (var i = 0; i < connector.destination.length; i++) {
        var destination_name = connector.destination[i];
        var target = self.connectors[destination_name];
        if (target) {
          links.push({
            source : connector,
            target : target,
            xStart : connector.xConnector,
            yStart : connector.yConnector,
            xEnd : target.xConnector,
            yEnd : target.yConnector,
            xMiddle : ((target.xConnector - connector.xConnector) / 2),
            yMiddle : ((target.yConnector - connector.yConnector) / 2)
          });
        } else {
          console.log("missing target destination ", destination_name);
        }
      }
    }
    return links;
  },[]);

  // diagonal from d3 does not allow you to control the
  // initial direction of the curve so we make out own.
  //
  // Desired:
  //      .......x
  //      .
  // x.....
  //
  // D3
  //             x
  //             .
  // .............
  // .
  // x
  var leftToRightDiagonal = function(link) {
    var path = [
      "M", link.xStart, ',', link.yStart, ' C',
      link.xStart + link.xMiddle, ',', link.yStart, ' ',
      link.xStart + link.xMiddle, ',', link.yEnd, ' ',
      link.xEnd, ',', link.yEnd].join('');
    return path;
  };

  gxDiagram.selectAll('path.connector')
    .data(links)
    .enter()
    .append("path")
    .classed("connector", true)
    // RIFT-2806 Chrome 37.0.2062
    .attr("d", leftToRightDiagonal);

  return links;
};
