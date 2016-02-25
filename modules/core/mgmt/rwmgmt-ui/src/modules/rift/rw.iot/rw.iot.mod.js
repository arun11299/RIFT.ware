(function(window, angular) {
  angular.module('rw.iot',['rwHelpers']);
  require('../rw.helper/rw.helper.mod.js');
  require('./directives/rw.iot.app_server_panel.dir.js');
  require('./directives/rw.iot.army_summary.dir.js');
    require('./directives/rw.iot.app_client_panel.dir.js');
    require('./directives/rw.iot.app_client-control.dir.js');
    require('./directives/rw.iot.app_client-controller.dir.js');
  require('./factory/rw.iot.app_server.factory.js')
  require('./factory/rw.iot.device_army.factory.js')
  require('./factory/rw.iot.slb.factory.js')
  require('./directives/rw.iot.app_server_details.dir.js');
  require('./directives/rw.iot-cag_panel.dir.js');
  require('./directives/rw.iot.premise_gw_panel.dir.js');
  require('./directives/rw.iot.army_details.dir.js');
  require('./directives/rw.iot.app_server_summary.dir.js');
})(window, window.angular);
