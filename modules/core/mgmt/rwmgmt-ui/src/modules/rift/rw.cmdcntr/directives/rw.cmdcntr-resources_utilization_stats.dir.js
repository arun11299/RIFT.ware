module.exports = (function(window, angular) {
  "use strict";
  angular.module('cmdcntr')
  .directive('resourcesUtilizationStats', resourcesUtilizationStats)
  .filter('showNodeIcon', showNodeIcon)
  .filter('getNodeClass', getNodeClass);

  function resourcesUtilizationStats() {

    function Controller () {
      this.created();
    };

    Object.defineProperty(Controller.prototype, "vcsNode", {
      enumerable: true,
      configurable: true,
      get: function() {
        return this._vcsNode;
      },
      set: function(val) {
        this._vcsNode = val;
        this.nodeChanged();
      }
    });

    Object.defineProperty(Controller.prototype, "theme", {
      enumerable: true,
      configurable: true,
      get: function() {
        return this._theme;
      },
      set: function(val) {
        this._theme = val;
        // do other stuff
      }
    });

    angular.extend(Controller.prototype, {
      created: function () {
        this.theme = rw.theme;
      },

      nodeChanged: function () {
        if (this._vcsNode) {
          this.targetNodes = this.nodeChildren(this._vcsNode);
        }
      },

      nodeStateString: function(target) {
        var children = this.nodeChildren(target);
        var up = 0;
        var chiledrenLen = 0;
        for (var i = 0; i < children.length; i++) {
          if(!children[i].children) {
            if (children[i].state == 'STARTED') {
              up++;
            }
            chiledrenLen++;
          }
        }
        return up + '/' + chiledrenLen + ' Up'
      },

      nodeChildren: function(node) {
        var ret = [node];
        if ('children' in node) {
          for (var i = 0; i < node.children.length; i++) {
            if(node.children[i].component_name == "RW_VM_CLI") {
              ret.splice(1, 0, node.children[i] )
            } else {
              ret = ret.concat(this.nodeChildren(node.children[i]));
            }

          }
        }

        return ret;
      },

      status: function(node) {
        var active = 0;
        var children = this.nodeChildren(node);
        if (children.length == 1) {
          return children[0].state;
        }
        for (var i = 0; i < children.length; i++) {
          if (children[i].state == "STARTED") {
            active++;
          }
        }
        return String(active) + "/" + String(children.length) + " Up"
      }
    });

    return {
      restrict: 'AE',
      templateUrl: '/modules/views/rw.cmdcntr-resources_utilization_stats.tmpl.html',
      scope: {
        vcsNode: '='
      },
      replace: true,
      controller: Controller,
      controllerAs: 'resourcesUtilizationStats',
      bindToController: true
    }
  };

  function showNodeIcon() {
    return function(node) {
      switch (rw.vcs.nodeType(node)) {
        case 'rwcolony':
          return '#icon-colony';
        case 'rwcluster':
          return '#icon-cluster';
        default:
          return '#icon-vm2';
      }
    }
  };

  function getNodeClass() {
    return function(node) {
      switch (rw.vcs.nodeType(node)) {
        case 'rwcolony':
          return 'colony';
        case 'rwcluster':
          return 'cluster';
        default:
          return 'vm2';
      }
    }
  };

})(window, window.angular);
