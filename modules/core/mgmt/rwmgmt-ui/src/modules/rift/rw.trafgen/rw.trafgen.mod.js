/**
 * Command Center Module
 * @fileoverview Command Center Module parent
 */
(function(window, angular) {
  "use strict";
  angular.module('trafgen', ['rwHelpers', 'dispatchesque', 'cmdcntr', 'uiModule'])
  require('./factory/rw.trafgen-controller.fctry');
  require('./factory/rw.trafgen.factory.js');
  require('./directives/rw.trafgen-dashboard_traffic_generator.dir.js');
  require('./directives/rw.trafgen-control.dir.js');
  require('./directives/rw.trafgen-controller.dir.js');
  require('./directives/rw.trafgen-details.dir.js');
  require('./directives/rw.trafgen-summary.dir.js');
  require('./directives/rw.trafgen-dashboard_panel.dir.js');
  require('./directives/rw.trafgen-dashboard_ip_traffic_sink.dir.js');

})(window, window.angular);
