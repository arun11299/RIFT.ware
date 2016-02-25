/**
 * Command Center Module
 * @fileoverview Command Center Module parent
 */

(function(window, angular) {
  "use strict";

  angular.module('slbalancer', ['rwHelpers', 'dispatchesque', 'uiModule', 'cmdcntr']);

 require('./directives/rw.slbalancer-config.dir.js');
 require('./directives/rw.slbalancer-dashboard_panel.dir.js');
 require('./directives/rw.slbalancer-details.dir.js');
 require('./directives/rw.slbalancer-summary.dir.js');
 require('./factory/rw.slbalancer.factory.js');

})(window, window.angular);
