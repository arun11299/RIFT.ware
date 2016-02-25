
rw.FpathAggregatorMixin = function(fpathModel, fabricOnly) {
  this.fabricOnly = fabricOnly || false;
  this.model = fpathModel;
  this.model.addEventListener("update", this.populate.bind(this));
};

rw.FpathAggregatorMixin.prototype = {
  aggregateChildren: function (o, children) {
    if (!children) {
      return;
    }
    o.speed = rw.math.total(children, 'speed');
    o.rx_rate_mbps = rw.math.total(children, 'rx_rate_mbps');
    o.tx_rate_mbps = rw.math.total(children, 'tx_rate_mbps');
    o.rx_rate_pps = rw.math.total(children, 'rx_rate_pps');
    o.tx_rate_pps = rw.math.total(children, 'tx_rate_pps');
    o.input_errors = rw.math.total(children, 'input_errors');
    o.output_errors = rw.math.total(children, 'output_errors');
    o.linkStateUp = 0;
    o.portCount = 0;
    for (var i = 0; i < children.length; i++) {
      if ('linkStateUp' in children[i]) {
        o.linkStateUp += children[i].linkStateUp;
        o.portCount += children[i].portCount;
      } else if ('link-state' in children[i]) {
        if (children[i]['link-state'] == 'up') {
          o.linkStateUp += 1;
        }
        o.portCount += 1;
      }
    }
    o.linkState = (o.linkStateUp == o.portCount ? 'up' : 'down');
    o.linkStateString = o.linkStateUp + "/" + o.portCount + ' Up';
    o.time = this.model.t;
    this.model.elaboratePortStats(o);
  },

  getVmPorts: function(vm) {
    if (this.fabricOnly) {
      if ('fabric' in vm && 'port' in vm.fabric) {
        return vm.fabric.port;
      }
    } else {
      return vm.port;
    }
    return null;
  }
};

rw.FpathVcs = function(fpathModel, sector, fabricOnly) {
  rw.FpathAggregatorMixin.call(this, fpathModel, fabricOnly);
  this.sector = sector;
  return this;
};

rw.FpathVcs.prototype = Object.create(rw.FpathAggregatorMixin.prototype);
rw.FpathVcs.prototype.constructor = rw.FpathAggregatorMixin;
rw.FpathVcs.prototype.populate = function() {
  var self = this;
  _.each(self.sector.allVms(), function (vm) {
    var ports = self.getVmPorts(vm);
    if (ports) {
      self.model.populate(ports);
      self.aggregateChildren(vm, ports);
    }
  });
  _.each(this.sector.allClusters(), function (cluster) {
    self.aggregateChildren(cluster, cluster.vm);
  });
  _.each(this.sector.allColonies(), function (colony) {
    self.aggregateChildren(colony, colony.collection);
  });
  self.aggregateChildren(self.sector, self.sector.collection);
};

rw.FpathVnf = function(fpathModel, vnfServices, fabricOnly) {
  rw.FpathAggregatorMixin.call(this, fpathModel, fabricOnly);
  this.services = vnfServices;
  return this;
};

rw.FpathVnf.prototype = Object.create(rw.FpathAggregatorMixin.prototype);
rw.FpathVnf.prototype.constructor = rw.FpathAggregatorMixin;
rw.FpathVnf.prototype.populate = function() {
  var self = this;
  _.each(self.services, function (service) {
    if (self.fabricOnly) {
      if ('fabric' in service && 'vm' in service.fabric) {
        _.each(service.fabric.vm, function (vm) {
          var ports = self.getVmPorts(vm);
          if (ports) {
            self.model.populate(ports);
            self.aggregateChildren(vm, ports);
          }
        });
        self.aggregateChildren(service, service.fabric.vm);
      }
    } else {
      _.each(service.connector, function (connector) {
        _.each(connector.interface, function (iface) {
          self.model.populate(iface.port);
          self.aggregateChildren(iface, iface.port);
        });
        self.aggregateChildren(connector, connector.interface);
      });
      self.aggregateChildren(service, service.connector);
    }
  });
};

/**
 * Adds up traffic for all ports, external and fabric. Useful only in very limited
 * situations.
 */
rw.FpathKitchenSink = function(fpathModel, obj, onUpdate) {
  rw.FpathAggregatorMixin.call(this, fpathModel, true);
  this.obj = obj;
  this.onUpdate = onUpdate;
  return this;
};

rw.FpathKitchenSink.prototype = Object.create(rw.FpathAggregatorMixin.prototype);
rw.FpathKitchenSink.constructor = rw.FpathAggregatorMixin;
rw.FpathKitchenSink.prototype.populate = function() {
  this.aggregateChildren(this.obj, _.values(this.model.ports));
  if (this.onUpdate) {
    this.onUpdate();
  }
};
