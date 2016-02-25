/**
 * UE-Widgets module
 * @fileoverview UE-Widgets module parent
 *
 * General UI widgets.
 */
(function(window, angular) {

  angular.module('uiModule',['ui.bootstrap','ui.codemirror']);
  require('./component/rw.scroll-graph.dir.js');
  require('./component/rw.ui-gauge.dir.js');
  require('./component/slider.js');
  require('./directives/geo-map.js');
  require('./directives/rw.bullet.dir.js');
  require('./directives/rw.bullet2.dir.js');
  require('./directives/rw.ui-bargraph.dir.js');
  require('./directives/rw.ui-gaugeset.dir.js');
  require('./directives/rw.ui-donut.dir.js');
  require('./directives/rw.ui-scatter.dir.js');
  require('./directives/scriptbox-directive.js');
  require('./directives/splitter.js');
  require('./directives/tabs.js');

})(window, window.angular);
