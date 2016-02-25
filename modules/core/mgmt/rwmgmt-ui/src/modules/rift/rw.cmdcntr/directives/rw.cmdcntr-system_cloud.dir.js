module.exports = (function(window, angular){
  "use strict";

  /**
   * <system-cloud> -
   */
  var controller = function($scope, radio, vcsResourcesFactory, portStateFactory, vcsFactory, $timeout) {
    var self = this;
    var appChannel = radio.channel('appChannel');
    this.created();
    var listeners = [];
    this.fireUpdateValues = function() {
      $scope.$broadcast("scatter-values-update");
    };
    portStateFactory.attached().done(function() {
        listeners.push(appChannel.on('port-state-update', function() {
          $timeout(self.update.bind(self));
        }, self));
      }
    );

    vcsFactory.attached().done(function() {
      self.sector = vcsFactory.sector;
      self.selectVcs(self.sector);
      vcsResourcesFactory.attached('sector', self.sector);
      listeners.push(appChannel.on('vcs-resources-updated', function(){
        $timeout(function() {
          $scope.$apply();
        });
      }, self));
    });

    rw.BaseController.call(this);
  };

  controller.prototype = {

    scatterColors: {
      'cpu': '#46b94b',
      'memory': '#fa8628'
    },

    markers: [{
      latLng: [42.36, -71.06],
      name: 'Boston'
    }],

    created: function () {
      this.node = {};
      this.bps = {
        utilization: 0,
        capacity: 0,
        percent: 0
      };
      this.scatterModel = [];
      this.selectCpu();
    },

    percent: function (metric) {
      return Math.min(metric ? Math.round(100 * metric.utilization / metric.capacity) : 0, 99);
    },

    selectCpu : function() {
      this.showCpu = true;
      this.showMemory = false;
      this.scatterXAxis = 'cpu';
      this.scatterPlotColor = this.scatterColors[this.scatterXAxis];
      this.resetData();
    },

    selectMemory : function() {
      this.showCpu = false;
      this.showMemory = true;
      this.scatterXAxis = 'memory';
      this.scatterPlotColor = this.scatterColors[this.scatterXAxis];
      this.resetData();
    },

    selectVcs: function (node) {
      if (this.sector) {
        this.node = node;
        this.vms = this.sector.vms(node);
        this.resetData();
      }
    },

    resetData: function() {
      if (typeof(this.vms) === "undefined") {
        return;
      }
      this.scatterModel = new Array(this.vms.length);
      this.update();
    },

    update: function() {
      if (typeof this.vms === "undefined") {
        return;
      }
      var self = this;
      for(var i = 0; i < this.vms.length; i++) {
        var vm = this.vms[i];

        var bps_percent;
        if ('speed' in vm) {
          bps_percent = Math.round(100 * (vm.tx_rate_mbps/vm.speed));
        } else {
          // RIFT-5577 - vms that don't use fpath won't have metrics so set to 0
          bps_percent = 0;
        }

        this.scatterModel[i] = this.scatterModel[i] || {};
        this.scatterModel[i].label = vm.instance_name;
        this.scatterModel[i].x = (self.scatterXAxis in vm ? vm[self.scatterXAxis].percent : 0);
        this.scatterModel[i].y = bps_percent;
        this.scatterModel[i].size = 6;
      }

      this.fireUpdateValues();

      this.vmCount = this.vms.length;
      this.bps.utilization = this.node.tx_rate_mbps;
      this.bps.capacity = this.node.speed;
      this.bps.percent = Math.round(100 * (this.bps.utilization / this.bps.capacity));
    }
  };

  angular.module('cmdcntr')
    .directive('systemCloud', function() {
      return {
        restrict: 'AE',
        templateUrl: '/modules/views/rw.cmdcntr-system_cloud.tmpl.html',
        controller: controller,
        controllerAs : "systemCloud",
        bindToController : true,
        replace: true
      };
    });

})(window, window.angular);
