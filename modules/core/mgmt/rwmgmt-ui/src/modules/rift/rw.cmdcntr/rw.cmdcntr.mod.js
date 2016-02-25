/**
 * Command Center Module
 * @fileoverview Command Center Module parent
 */
module.exports = (function(window, angular) {
  "use strict";

  angular.module('cmdcntr', ['rwHelpers', 'dispatchesque', 'uiModule', 'ui.router'])
    .config(function($stateProvider) {
      /**
      // Seriously need a better way of doing this
      // At the very least should be refactored
      **/
  /** Creates the $rw global for storing the main app and navigation */
  if (typeof window.$rw === 'undefined') {
    window.$rw = {
      nav: []
    };
  }



     window.$rw.nav.push({
        module: 'cmdcntr',
        name: "Traffic",
        icon: "icon-control-panel"
      });
      $stateProvider.state('traffic', {
        url: '/traffic',
        replace: true,
        template: '<traffic-page></traffic-page>'
      });

     window.$rw.nav.push({
        module: 'cmdcntr',
        name: "Interfaces",
        icon: "icon-graph"
      });
      $stateProvider.state('interfaces', {
        url: '/interfaces',
        replace: true,
        template: '<network-page></network-page>'
      });


      window.$rw.nav.push({
        module: 'cmdcntr',
        name: "Topology",
        icon: "icon-control-panel"
      });

      $stateProvider.state('topology', {
        url: '/topology',
        replace: true,
        template: '<topology></topology>'
      });


      window.$rw.nav.push({
        module: 'cmdcntr',
        name: "Resources",
        icon: "icon-cloud-server"
      });
      $stateProvider.state('resources', {
        url: '/resources',
        replace: true,
        template: '<resources></resources>'
      });

    })

require('./directives/rw-cmdcntr-connector_summary.dir.js');
require('./directives/rw.cmdcntr-header.dir.js');
require('./directives/rw.cmdcntr-iface_details.dir.js');
require('./directives/rw.cmdcntr-network_diagram.dir.js');
require('./directives/rw.cmdcntr-network_page.dir.js');
require('./directives/rw.cmdcntr-network_page.dir.js');
require('./directives/rw.cmdcntr-resources.dir.js');
require('./directives/rw.cmdcntr-resources_detail.dir.js');
require('./directives/rw.cmdcntr-resources_diagram.dir.js');
require('./directives/rw.cmdcntr-resources_utilization_stats.dir.js');
require('./directives/rw.cmdcntr-status_led.dir.js');
require('./directives/rw.cmdcntr-system_cloud.dir.js');
require('./directives/rw.cmdcntr-topology.dir.js');
require('./directives/rw.cmdcntr-traffic.dir.js');
require('./directives/rw.cmdcntr-traffic_diagram.dir.js');
require('./directives/rw.cmdcntr-traffic_stats.dir.js');
require('./directives/rw.cmdcntr-vcs_table.dir.js');

require('./factory/rw.cmdcntr-port_state.factory.js');
require('./factory/rw.cmdcntr-vcs.factory.js');
require('./factory/rw.cmdcntr-vcs_resources.factory.js');
require('./factory/rw.cmdcntr-vnf.factory.js');

require('./js/fpath-aggregator.js');
require('./js/network-diagram.js');




})(window, window.angular);
