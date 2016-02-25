(function(window, angular) {
  "use strict";
  angular.module('cmdcntr')
  .directive('resourcesDetail', resourcesDetail);

  function resourcesDetail() {

    function Controller () {
    };

    Object.defineProperty(Controller.prototype, "vcsNode", {
      enumerable: true,
      configurable: true,
      get: function() {
        return this._vcsNode;
      },
      set: function(val) {
        if (val) {
          this._vcsNode = val;
          this.nodeChanged();
        }
      }
    });

    Object.defineProperty(Controller.prototype, "fabricNode", {
      enumerable: true,
      configurable: true,
      get: function() {
        return this._fabricNode;
      },
      set: function(val) {
        this._fabricNode = val;
      }
    });

    Object.defineProperty(Controller.prototype, "processes", {
      enumerable: true,
      configurable: true,
      get: function() {
        return this._processes;
      },
      set: function(val) {
        this._processes = val;
        // do other stuff
      }
    });

    Object.defineProperty(Controller.prototype, "tasklets", {
      enumerable: true,
      configurable: true,
      get: function() {
        return this._tasklets;
      },
      set: function(val) {
        this._tasklets = val;
        // do other stuff
      }
    });

    _.extend(Controller.prototype, {
      percent: function(metric) {
        return Math.min(metric ? Math.round(100 * metric.utilization / metric.capacity) : 0, 99);
      },

      flipTab: function(e) {
        if (!e.detail.isSelected) {
          return;
        }
        var tabs = this.shadowRoot.querySelectorAll('div.tab');
        var selected = e.detail.item.attributes['data-tab'].value;
        _.each(tabs, function(tab) {
          tab.classList.toggle('tab-on', tab.id == selected);
        });
      },

      nodeChanged: function() {
        this.processes = this.collectProcesses(this._vcsNode, []);
        this.tasklets = this.collectTasklets(this._vcsNode, []);
        this.interfaces = this.collectInterfaces(this._vcsNode, []);
      },

      collectProcesses: function(node, processes) {
        if (node.component_type == 'RWPROC') {
          processes.push(node);
        } else {
          var children = node.children;
          if (node.component_type == 'RWPROC') {
            children = node.tasklet;
          } else if (node.component_type == 'RWVM') {
            children = node.process;
          }
          if (children) {
            for (var i = 0; i < children.length; i++) {
              this.collectProcesses(children[i], processes);
            }
          }
        }
        return processes;
      },

      collectTasklets: function(node, tasklets) {
        if (node.component_type == 'RWTASKLET') {
          tasklets.push(node);
        } else {
          var children = node.children;
          if (node.component_type == 'RWPROC') {
            children = node.tasklet;
          } else if (node.component_type == 'RWVM') {
            children = node.process;
          }
          if (children) {
            for (var i = 0; i < children.length; i++) {
              this.collectTasklets(children[i], tasklets);
            }
          }
        }
        return tasklets;
      },

      collectInterfaces: function(node, ifaces) {
        if (node.component_type == 'RWVM') {
          if (node.connector) {
            for (var i = 0; i < node.connector.length; i++) {
              ifaces.push.apply(ifaces, node.connector[i].interface);
            }
          }
        } else {
          if (node.children) {
            for (var i = 0; i < node.children.length; i++) {
              if(node.children[i].connector){
                for (var j = 0; j < node.children[i].connector.length; j++) {
                  ifaces.push.apply(ifaces, node.children[i].connector[j].interface);
                }
              } else {
                this.collectInterfaces(node.children[i], ifaces)
              }
            }
          }
        }
        return ifaces;
      }
    });

    return {
      restrict: 'AE',
      templateUrl: '/modules/views/rw.cmdcntr-resources_detail.tmpl.html',
      scope: {
        vcsNode: '=',
        fabricNode: '='
      },
      replace: true,
      controller: Controller,
      controllerAs: 'resourcesDetail',
      bindToController: true
    }
  };
})(window, window.angular);
