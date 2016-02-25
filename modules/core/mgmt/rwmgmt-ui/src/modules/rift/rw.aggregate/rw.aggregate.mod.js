/**
 * Command Center Module
 * @fileoverview Command Center Module parent
 */
(function(window, angular) {
  "use strict";

  angular.module('rw.aggregate', ['rwHelpers', 'dispatchesque', 'uiModule', 'ui.router'])
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
        module: 'rw.aggregate',
        name: "Dashboard",
        icon: "icon-control-panel"
      });
      $stateProvider.state('dashboard', {
        url: '/dashboard',
        replace: true,
        template: '<dashboard-page class="viewport__body"></dashboard-page>'
      });
    })


require('./directives/rw.aggregate-dashboard_page.dir.js');

})(window, window.angular);
