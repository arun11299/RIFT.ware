
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var React = require('react');
var LoginScreen = require('./login.jsx');
angular.module('login', ['ui.router'])
    .config(function($stateProvider) {

     $rw.nav.push({
        module: 'login',
        name: "Login"
      });

      $stateProvider.state('login', {
        url: '/login',
        replace: true,
        template: '<login-screen></login-screen>'
      });

})
.directive('loginScreen', function() {
  return {
      restrict: 'AE',
      controller: function($element) {
        function reactRender() {
          React.render(
            React.createElement(LoginScreen, null)
            ,
            $element[0]
          );
        }
        reactRender();
      }
    };
})
    ;
