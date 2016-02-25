module.exports = (function(window, angular) {
  "use strict";

  /**
   * <traffic-page> -
   */
  var controller = function($scope, vnfFactory, radio, portStateFactory, $rootScope, $element, $compile, $timeout) {
    var self = this;
    var appChannel = radio.channel('appChannel');
    this.$element = $element;
    $scope.l10n = rw.ui.l10n;
    var listeners = [];
    $.when(vnfFactory.attached(), portStateFactory.attached()).done(function() {
      var aggregator = new rw.XFpathVnf(portStateFactory, vnfFactory.services, false);
      listeners.push(appChannel.on('port-state-update', function() {
        $timeout(function() {
          aggregator.populate();
        })
      }, self));
      $scope.vnfNode = vnfFactory.services[0];
      $scope.services = vnfFactory.services;
      listeners.push(appChannel.on("select-service", function(service) {
        $timeout(function() {
          self.selectService(service);
        });
      }, self));
      $timeout(function() {
        $scope.$apply();
      });
    });

    // $scope.$on('$stateChangeStart', function() {
    //   portStateFactory.detached();
    //   vnfFactory.detached();
    //   _.each(listeners, function(listener) {
    //     listener.cancel();
    //   });
    //   listeners.length = 0;
    // });

    this.compile = function(elem) {
      return $compile(elem)($scope)[0];
    };
     rw.BaseController.call(this);
  };

  controller.prototype = {

    selectService: function(service) {
      this.selectedService = service;
      this.buildDetailsComponent(service);
    },

    // NOTE: Ideally knowledge of the type of VNF is abstracted out. This would have to use
    // plugin module loader to create proper widget(s).
    buildDetailsComponent: function(service) {
      var tabs = [];
      var portContent = document.createElement("traffic-stats");
      switch (this.selectedService.type) {
        case 'trafsimclient':
        case 'trafsimserver':
          var detailsContent = document.createElement("trafsim-details");
          tabs.push({
            label: 'Stats',
            element: detailsContent
          });
          tabs.push({
            label: 'Ports',
            element: portContent
          });
          break;
        case 'ltemmesim':
        case 'ltegwsim':
          detailsContent = document.createElement("lte-details");
          tabs.push({
            label: 'Stats',
            element: detailsContent
          });
          tabs.push({
            label: 'Ports',
            element: portContent
          });
          break;
        case 'trafgen':
        case 'trafsink':
          detailsContent = document.createElement("trafgen-details");
          tabs.push({
            label: 'Stats',
            element: detailsContent
          });
          tabs.push({
            label: 'Ports',
            element: portContent
          });
          break;
        case 'slbalancer':
          detailsContent = document.createElement("slbalancer-details");
          tabs.push({
            label: 'Stats',
            element: detailsContent
          });
          var configContent = document.createElement("slbalancer-config");
          tabs.push({
            label: 'Config',
            element: configContent
          });
          tabs.push({
            label: 'Ports',
            element: portContent
          });
          break;
        default:
          tabs.push({
            label: 'Ports',
            element: portContent
          });
          break;
      }

      var tabsElem = this.compile(this.createTabs(tabs));
      var container = $(this.$element).find("#traffic-details-container");
      container.empty();
      container.append(tabsElem);
    },

    createTabs: function(tabs) {
      var tabsElem;
      if (tabs.length == 1) {
        tabsElem = tabs[0].element;
      } else {
        tabsElem = document.createElement("rw-tabs");
        _.each(tabs, function(tab) {
          var pane = document.createElement("pane");
          pane.setAttribute("title", tab.label);
          pane.appendChild(tab.element);
          tabsElem.appendChild(pane);
        });
      }
      _.each(tabs, function(tab) {
        if (tab.label === 'Ports') {
          tab.element.setAttribute("vnf-node", "trafficPage.selectedService");
        } else {
          tab.element.setAttribute("service", "trafficPage.selectedService");
        }
      });

      return tabsElem;
    },

    safePrefix: function(val, prefix) {
      if (val === undefined || val === null || val.length == 0) {
        return '';
      }
      return prefix + val;
    },

    resetTotals: function() {
      if (this.selectedService && this.selectedService.connector) {
        this.resetFpathTotals(this.selectedService);
        switch (this.selectedService.type) {
          case 'trafsimclient':
          case 'trafsimserver':
            this.resetTrafsimTotals();
            break;
          case 'ltemmesim':
          case 'ltegwsim':
            this.resetLteTotals(this.selectedService);
            break;
          case 'trafgen':
          case 'trafsink':
            this.resetTrafgenTotals(this.selectedService);
            break;
        }
      }
    },

    resetTrafgenTotals: function(service) {
      for (var i = 0; i < service.connector.length; i++) {
        var data = {
          input: {
            colony: {
              name: service.connector[i].interface[0].colonyId,
              'trafgen-counters': {
                'all': ""
              }
            }
          }
        };
        rw.api.rpc('/api/operations/fpath-clear', data).done(function() {
          console.log('cleared trafgen stats for ', service.name);
        });
      }
    },

    resetLteTotals: function(service) {
      var data = {
        input: {
          colony: {
            name: service.ltesim.colonyId,
            'trafsim-service': {
              name: service.ltesim.name
            }
          }
        }
      };
      rw.api.rpc('/api/operations/trafsim-clear', data).done(function() {
        console.log('cleared lte stats for ', service.name);
      });
    },

    resetTrafsimTotals: function(service) {
      // backend doesn't support yet 4/2/15
      console.log('Trafsim stats reset not implemented');
    },

    resetFpathTotals: function(service) {
      for (var i = 0; i < service.connector.length; i++) {
        console.log(service.connector[i]);
        var iface = service.connector[i].interface[0];
        var data = {
          input: {
            colony: {
              name: iface.colonyId,
              counters: {
                "all": ""
              }
            }
          }
        };
        rw.api.rpc('/api/operations/fpath-clear', data).done(function() {
          console.log('cleared fpath stats for ', iface.name);
        });
      }
    }
  };

  angular.module('cmdcntr')
    .directive('trafficPage', function() {
      return {
        restrict: 'AE',
        templateUrl: '/modules/views/rw.cmdcntr-traffic.tmpl.html',
        controller: controller,
        controllerAs: "trafficPage",
        bindToController: true,
        replace: true
      };
    });

})(window, window.angular);
