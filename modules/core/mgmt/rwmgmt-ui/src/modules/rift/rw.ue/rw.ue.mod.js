(function(window, angular) {
  "use strict";
  angular.module('ue', ['rwHelpers', 'dispatchesque', 'uiModule']);

  require('./directives/rw.ue-dashboard_ue.dir.js');
  require('./directives/rw.ue-dashboard_ue_panel.dir.js');
  require('./directives/rw.ue-lte_details.dir.js');
  require('./directives/rw.ue-lte_summary.dir.js');

  require('./services/rw.ue.svc.js');
})(window, window.angular);
