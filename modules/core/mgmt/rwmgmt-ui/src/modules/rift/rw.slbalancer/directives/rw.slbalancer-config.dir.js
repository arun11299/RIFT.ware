/**
 * Command Center Module
 * @fileoverview Command Center Module parent
 */
(function(window, angular) {
  "use strict";

  /**
   * <slbalancer-config>
   */
  var controller = function($scope, slbFactory) {
    var self = this;
    slbFactory.attached().done(function() {
      self.slb = slbFactory.slb;
      self.slbChanged();
      $scope.$apply();
    });
  };

  controller.prototype = {

    slbChanged: function() {
      var isFunction = /^.*-function$/;
      this.funcs = [];
      for (var prop in this.slb) {
        if (prop.match(isFunction)) {
          this.funcs.push({name : prop, children : this.getChildren(this.slb[prop])});
        }
      }
    },

    getChildren: function(obj) {
      var children = [];
      for (var prop in obj) {
        if (typeof(obj[prop]) == 'object') {
          var grandChildren = this.getChildren(obj[prop]);
          children.push({name : prop, children : grandChildren});
        } else {
          children.push({name : prop, value : obj[prop]});
        }
      }
      return children;
    }
  };

  angular.module('slbalancer')
    .directive('slbalancerConfigProperty', function(RecursionHelper) {
      return {
        restrict: 'E',
        templateUrl: '/modules/views/rw.slbalancer-config_property.tmpl.html',
        replace: true,
        scope: {
          children : '='
        },
        compile: function(element) {
          // Use the compile function from the RecursionHelper,
          // And return the linking function(s) which it returns
          return RecursionHelper.compile(element);
        }
      };
    })
    .directive('slbalancerConfig', function() {
      return {
        restrict: 'AE',
        templateUrl: '/modules/views/rw.slbalancer-config.tmpl.html',
        replace: true,
        controller : controller,
        controllerAs : 'slbalancerConfig',
        bindToController : true,
        scope : {
          service : '='
        }
      };
    });

})(window, window.angular);

