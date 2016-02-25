module.exports = (function(window, angular) {
  "use strict";
  angular.module('cmdcntr')
  .directive('ifaceDetails', ifaceDetails)
  .filter('duplextotxflow', duplexToTxFlow)
  .filter('duplextorxflow', duplexToRxFlow);

  function ifaceDetails() {

    // no special processing required in controller
    // but left here to provide as an example for others
    function Controller () {
    };

    Object.defineProperty(Controller.prototype, "iface", {
      enumerable: true,
      configurable: true,
      get: function() {
        return this._iface;
      },
      set: function(val) {
        this._iface = val;
      }
    });

    Object.defineProperty(Controller.prototype, "port", {
      enumerable: true,
      configurable: true,
      get: function() {
        return this._portDescription;
      },
      set: function(val) {
        if (!val || val['descr-string'] == null || Object.keys(val['descr-string']).length === 0) {
          this._portDescription = '';
        } else {
          this._portDescription = JSON.stringify(val['descr-string']);
        }
      }
    });

    angular.extend(Controller.prototype, {
    });

    return {
      restrict: 'AE',
      templateUrl: '/modules/views/rw.cmdcntr-iface_details.tmpl.html',
      scope: {
        iface: '=',
        port: '='
      },
      replace: true,
      controller: Controller,
      controllerAs: 'ifaceDetails',
      bindToController: true
    }
  };

  function duplexToTxFlow() {
    return function(input) {
      input = input || '';
      var output = 'Off';
      if (input === 'full-duplex') {
        output = 'On';
      }
      return output;
    }
  };

  function duplexToRxFlow() {
    return duplexToTxFlow();
  };
})(window, window.angular);
