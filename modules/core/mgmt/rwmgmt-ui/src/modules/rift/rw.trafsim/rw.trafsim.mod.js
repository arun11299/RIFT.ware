/**
 * Command Center Module
 * @fileoverview Command Center Module parent
 */
(function(window, angular) {
  "use strict";

  angular.module('trafsim', ['rwHelpers', 'dispatchesque', 'uiModule', 'cmdcntr'])

require('./directives/rw.trafsim-control.dir.js');
require('./directives/rw.trafsim-controller.dir.js');
require('./directives/rw.trafsim-dashboard_client.dir.js');
require('./directives/rw.trafsim-dashboard_server.dir.js');
require('./directives/rw.trafsim-dashboard_servers.dir.js');
require('./directives/rw.trafsim-details.dir.js');
require('./directives/rw.trafsim_summary.dir.js');

require('./factory/rw.trafsim.factory.js');

})(window, window.angular);
