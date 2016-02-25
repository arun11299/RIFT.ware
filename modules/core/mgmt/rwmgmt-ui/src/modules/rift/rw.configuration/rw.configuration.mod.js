/**
 * Configuration Module
 * @fileoverview Configuration Module parent
 */



(function(window, angular) {
  "use strict";

  angular.module('configuration', ['ui.router', 'rwHelpers'])
    .config(function($stateProvider) {
      try{
          window.$rw.nav.push({
        module: 'configuration',
        name: "Configuration",
        icon: "icon-control-panel"
      });
        } catch(e){
          console.log(e)
        }


      $stateProvider.state('configuration', {
        url: '/configuration',
        template: '<rw-configuration class="viewport__body"></rw-configuration>'
      });

    })

require('./directives/rw.config-viewer.js');
require('./directives/rw.configuration.js');

})(window, window.angular);
