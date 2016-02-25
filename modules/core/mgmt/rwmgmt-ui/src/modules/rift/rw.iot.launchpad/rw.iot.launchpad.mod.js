(function(window, angular) {
  angular.module('rw.iot.launchpad', ['radio']);
  // require('./directives/')
  require('./directives/rw.iot.launchpad-environment_card.dir.js');
  require('./directives/rw.iot.launchpad-environment_metric.dir.js');
  require('./factory/rw.iot.launchpad-factory.js')
})(window, window.angular)
