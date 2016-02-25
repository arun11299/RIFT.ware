/**
 * Command Center Module
 * @fileoverview Command Center Module parent
 */

(function(window, angular) {
  "use strict";

  angular.module('loadbal', ['rwHelpers', 'dispatchesque', 'cmdcntr'])

require('./factory/rw.loadbal.factory.js');
require('./directives/rw-loadbal-summary.dir.js');

})(window, window.angular);
