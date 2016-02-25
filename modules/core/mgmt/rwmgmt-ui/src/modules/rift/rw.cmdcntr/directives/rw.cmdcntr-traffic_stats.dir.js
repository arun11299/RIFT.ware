module.exports = (function(window, angular){
  "use strict";

  /**
   * <traffic-stats> -
   */
  var controller = function($scope, $element, radio, $timeout) {
    this.items = [];
    this.showLabels = false;
    this.paused = false;
    $scope.theme = rw.theme;
    var self = this;
    $scope.$watch(
      function() {
        return self.vnfNode;
      },
      function () {
        if (typeof(self.vnfNode) === 'undefined') {
          return;
        }
        self.vnfChanged();
    });
    $scope.$watch(
      function() {
        return self.vcsNode;
      },
      function() {
        if (typeof(self.vcsNode) === 'undefined') {
          return;
        }
        self.vcsChanged();
    });
  };

  controller.prototype = {
    open : true,

    vnfChanged : function() {
      this.items.length = 0;
      if (this.showFabric) {
        this.vnfFabricPortStats();
      } else {
        this.vnfExternalPortStats();
      }
      this.determineVisibleItems();
    },

    vcsChanged : function() {
      this.items.length = 0;
      this.pushVcsNode(this, this.vcsNode);
      this.items.reverse();
      this.determineVisibleItems();
    },

    vnfExternalPortStats : function() {
      var self = this;
      _.each(this.vnfNode.connector, function (connector) {
        connector.name = 'connector';
        var citem = {
          data: connector,
          label: connector.name,
          parent: self,
          open: false,
          icon: 'icon-connector',
          type: 'connector'
        };
        self.items.push(citem);
        _.each(connector.interface, function (iface) {
          var iitem = {
            data: iface,
            label: iface.name,
            parent: citem,
            open: false,
            icon: 'icon-interface-group',
            type: 'interface'
          };
          self.items.push(iitem);
          _.each(iface.port, function(port) {
            self.items.push({
              data: port,
              label: port.name,
              parent: iitem,
              open: false,
              icon: 'icon-port',
              type: 'port'
            });
          });
        });
      });
    },

    determineVisibleItems : function() {
      this.visibleItems = this.items.filter(function(item) {
        return item.parent.open;
      });
    },

    /*
     NOTE: We build list in reverse order so we can remove parents that have no children.  I.E. If a cluster
     has zero vms that have zero ports then don't push it to the items list, which can only be done
     if we push parents after we've recursed thru children.
     */
    pushVcsNode : function(parent, node) {
      var self = this;
      var n = 0;
      var item = {
        data: node,
        label: node.component_name || 'sector',
        parent: parent,
        icon: 'icon-connector',
        type: node.component_type
      };
      var forEachPush = function(parentItem, children) {
        var forEachN = 0;
        _.each(children, function(child) {
          // recursive!
          forEachN += self.pushVcsNode(parentItem, child);
        });
        return forEachN;
      };
      switch (rw.vcs.nodeType(node)) {
        case 'rwsector':
        case 'rwcolony':
          item.icon = 'icon-colony';
          n += forEachPush(item, node.collection);
          n += forEachPush(item, node.vm);
          break;
        case 'rwcluster':
          item.icon = 'icon-cluster';
          n += forEachPush(item, node.collection);
          n += forEachPush(item, node.vm);
          break;
        case 'RWVM':
          item.icon = 'icon-vm2';
          if (this.showFabric) {
            if ('fabric' in node && 'port' in node.fabric) {
              _.each(node.fabric.port, function(p) {
                n += 1;
                self.items.push({
                  data: p,
                  label: p.name,
                  parent: item,
                  open: false,
                  icon: 'icon-fabric',
                  type: 'port'
                });
              });
            }
          } else {
            _.each(node.port, function(p) {
              n += 1;
              self.items.push({
                data: p,
                label: p.name,
                parent: item,
                open: false,
                icon: 'icon-port',
                type: 'port'
              });
            });
          }
          break;
      }
      if (n > 0) {
        this.items.push(item);
        n += 1;
      }
      return n;
    },

    vnfFabricPortStats : function() {
      var self = this;
      _.each(this.vnfNode.vm, function(vm) {
        if ('fabric' in vm && 'port' in vm.fabric && vm.fabric.port.length > 0) {
          self.items.push({
            data: vm,
            label: vm.component_name,
            parent: self,
            open: false,
            icon: 'icon-vm2',
            type: vm.component_type
          });
          _.each(vm.fabric.port, function (port) {
            self.items.push({
              data: port,
              label: port.name,
              parent: vitem,
              open: false,
              icon: 'icon-fabric',
              type: 'fabric'
            });
          });
        }
      });
    },

    toggleExpand : function(item) {
      item.open = ! item.open;
      this.determineVisibleItems();
    },

    level : function(n) {
      var level = -1;
      var i = n.parent;
      while (i) {
        level += 1;
        i = i.parent;
      }
      return level;
    },

    hasChildren : function(n) {
      for (var i = 0; i < this.items.length; i++) {
        if (this.items[i].parent === n) {
          return true;
        }
      }
      return false;
    }
  };

  angular.module('cmdcntr')
    .directive('trafficStats', function() {
      return {
        restrict: 'AE',
        templateUrl: '/modules/views/rw.cmdcntr-traffic_stats.tmpl.html',
        controller: controller,
        controllerAs : 'trafficStats',
        bindToController: true,
        scope : {
          vnfNode : '=',
          vcsNode : '=',
          showFabric : '@',
          paused : '@?'
        },
        replace: true
      };
    });

})(window, window.angular);
