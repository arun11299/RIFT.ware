/**
 * Command Center Module
 * @fileoverview Command Center Module parent
 */
module.exports = (function(window, angular) {
  "use strict";

  rw.XFpathAggregatorMixin = function(portStateFactory, fabricOnly) {
    var self = this;
    this.fabricOnly = fabricOnly || false;
    this.portStateFactory = portStateFactory;
    //portStateFactory.attached().done(function() {
    //  self.populate();
    //});
  };

  rw.XFpathAggregatorMixin.prototype = {
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
      o.time = this.portStateFactory.t;
      this.portStateFactory.elaboratePortStats(o);
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

  rw.XFpathVcs = function(portStateFactory, sector, fabricOnly) {
    this.sector = sector;
    rw.XFpathAggregatorMixin.call(this, portStateFactory, fabricOnly);
    return this;
  };

  rw.XFpathVcs.prototype = Object.create(rw.XFpathAggregatorMixin.prototype);
  rw.XFpathVcs.prototype.constructor = rw.XFpathAggregatorMixin;
  rw.XFpathVcs.prototype.populate = function() {
    var self = this;
    _.each(self.sector.allVms(), function (vm) {
      var ports = self.getVmPorts(vm);
      if (ports) {
        self.portStateFactory.populate(ports);
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

  rw.XFpathVnf = function(portStateFactory, vnfServices, fabricOnly) {
    rw.XFpathAggregatorMixin.call(this, portStateFactory, fabricOnly);
    this.services = vnfServices;
    return this;
  };

  rw.XFpathVnf.prototype = Object.create(rw.XFpathAggregatorMixin.prototype);
  rw.XFpathVnf.prototype.constructor = rw.XFpathAggregatorMixin;
  rw.XFpathVnf.prototype.populate = function() {
    var self = this;
    _.each(self.services, function (service) {
      if (self.fabricOnly) {
        if ('fabric' in service && 'vm' in service.fabric) {
          _.each(service.fabric.vm, function (vm) {
            var ports = self.getVmPorts(vm);
            if (ports) {
              self.portStateFactory.populate(ports);
              self.aggregateChildren(vm, ports);
            }
          });
          self.aggregateChildren(service, service.fabric.vm);
        }
      } else {
        _.each(service.connector, function (connector) {
          _.each(connector.interface, function (iface) {
            self.portStateFactory.populate(iface.port);
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
  rw.XFpathKitchenSink = function(portStateFactory, obj, onUpdate) {
    rw.XFpathAggregatorMixin.call(this, portStateFactory, true);
    this.obj = obj;
    this.onUpdate = onUpdate;
    return this;
  };

  rw.XFpathKitchenSink.prototype = Object.create(rw.XFpathAggregatorMixin.prototype);
  rw.XFpathKitchenSink.constructor = rw.XFpathAggregatorMixin;
  rw.XFpathKitchenSink.prototype.populate = function() {
    this.aggregateChildren(this.obj, _.values(this.portStateFactory.ports));
    if (this.onUpdate) {
      this.onUpdate();
    }
  };

})(window, window.angular);
