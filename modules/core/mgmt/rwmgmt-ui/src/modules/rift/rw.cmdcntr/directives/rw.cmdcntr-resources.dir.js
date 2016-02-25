module.exports = (function(window, angular) {
  "use strict";

  angular.module('cmdcntr')
  .directive('resources', ['vcsFactory', 'portStateFactory', 'vcsResourcesFactory', 'radio', resources]);

  function resources() {

    function Controller (vcsFactory, portStateFactory, vcsResourcesFactory, radio, $scope, $timeout) {

      var self = this;
      var appChannel = radio.channel('appChannel');
      self.appChannel = appChannel;
      self.vcsFactory = vcsFactory;
      self.portStateFactory = portStateFactory;
      self.vcsResourcesFactory = vcsResourcesFactory;
      self.$scope = $scope;

      self.listeners = [];

      var vcsResourcesWatcher = appChannel.on('vcs-resources-updated', function() {
        $timeout(function() {
          self.updateMetrics()
        });
      }, self);

      self.listeners.push(vcsResourcesWatcher);

      self.listeners.push(appChannel.on('node-selected', function(node) {
        $timeout(function() {
          self.nodeSelected(node)
        });
      }, self));

      self.vcsFactory.attached().done(function() {
        $timeout(function() {
          self.updateVcs();
          self.vcsResourcesFactory.attached('sector', self.vcsFactory.sector);
        });
      });
      self.portStateFactory.attached();

      rw.BaseController.call(this);
    };


    Controller.prototype = Object.create(rw.BaseController.prototype, {
      TYPES: {
        configurable: false,
        value: {
          rwcolony : 'Colony',
          rwcluster : 'Cluster',
          RWVM : 'Virtual Machine'
        }
      },
      selectedNode: {
        enumerable: true,
        configurable: true,
        get: function() {
          return this._selectedNode;
        },
        set: function(val) {
          if (val) {
            this._selectedNode = val;
          }
        }
      },
      nodeTypeLabel: {
        enumerable: true,
        configurable: true,
        get: function() {
          return this._nodeTypeLabel;
        },
        set: function(val) {
          if (val) {
            this._nodeTypeLabel = val;
          }
        }
      }
    });

    Controller.prototype.nodeSelected = function(node) {
      this.selectedNode = node;
      this.nodeTypeLabel = this.TYPES[rw.vcs.nodeType(node)];
    };

    Controller.prototype.updateMetrics = function() {
      this.appChannel.trigger('diagram-update');
    };

    Controller.prototype.updateVcs = function() {
      var self = this;
      // On receiving vcs update, build fabric tree as well
      new rw.VcsVisitor(function(parent, node, i, listType) {
        var clone = _.clone(node);
        if (parent) {
          if (i === 0) {
            parent._fabricNode[listType] = [];
          }
          parent._fabricNode[listType][i] = clone;
        }
        node._fabricNode = clone;
        return node.component_type == 'RWVM' ? false : true;
      }).visit(self.vcsFactory.sector);

      // aggregate external port stats
      var externalPortStatsAggregator = new rw.XFpathVcs(self.portStateFactory, self.vcsFactory.sector, false);
      // aggregate fabric port stats
      var fabricPortStatsAggregator = new rw.XFpathVcs(self.portStateFactory, self.vcsFactory.sector._fabricNode, true);
      var count = 0;
      self.listeners.push(self.appChannel.on('port-state-update',
        function() {
          externalPortStatsAggregator.populate();
          fabricPortStatsAggregator.populate();
          //Hacky VCPU Count fix
          // self.vcsFactory.sector.collection[0].cpu.aggregate.ncores = self.vcsFactory.sector
          self.$scope.$apply();
        }, self
      ));
    };

    Controller.prototype.doCleanup = function() {
      // do additional cleanup here.
    };

    Controller.prototype.constructor = Controller;

    return {
      restrict: 'AE',
      templateUrl: '/modules/views/rw.cmdcntr-resources.tmpl.html',
      scope: {
        vcsNode: '=selectedNode'
      },
      replace: true,
      controller: Controller,
      controllerAs: 'resources',
      bindToController: true
    }
  };
})(window, window.angular);
