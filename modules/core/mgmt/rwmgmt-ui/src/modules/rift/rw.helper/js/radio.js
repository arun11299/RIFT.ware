window.Backbone = {};

var BackboneEventsStandalone = require('backbone-events-standalone');
window.Backbone.Events = BackboneEventsStandalone;

window.Backbone.Radio = require('backbone.radio');

module.exports = (function(window, angular){
  angular.module('radio', [])
    .factory('radio', function() {
      return window.Backbone.Radio;
    });
})(window, window.angular);
