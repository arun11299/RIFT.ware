(function(window, angular){
require('../js/jquery.circliful.min.js');
  "use strict";

  /**
   * <rw-donut>
   */
  var controller = function($scope, $element) {
    var self = this;
    this.value = 0;
    this.color = this.color || '#ff0000';
    this.archWidth = 28;
    this.dimension = 200;
    this.circliful = $($element);
    this.circliful.circliful();
    var debouncedUpdate = _.debounce(this.update.bind(this), 100);
    //var debouncedUpdate = this.update.bind(this);
    $scope.$watch(
      function() { return self.label; },
      debouncedUpdate);
    $scope.$watch(
      function() { return self.value; },
      debouncedUpdate);
  };

  controller.prototype = {
    update: function() {
      // creates a stutter effect, but widget doesn't support updating
      // https://github.com/pguso/jquery-plugin-circliful/issues/6
      this.circliful.empty().removeData();
      this.circliful.attr('data-percent', this.value);
      this.circliful.attr('data-text', this.value + '%');
      this.circliful.circliful();
    }
  };

  angular.module('uiModule')
    .directive('rwDonut', function() {
      return {
        restrict: 'AE',
        template: '' +
        '<div class="donut-mycircliful" data-dimension="140" data-info="{{donut.label}}" ' +
          'data-fontsize="20" data-fgcolor="{{donut.color}}"  data-bgcolor="#383838" ' +
          'data-border="#1f1f1f" data-type="half" data-width="{{donut.archWidth}}" ' +
          'data-animation-step="0" data-icon="fa-task"></div>',
        controller: controller,
        controllerAs: 'donut',
        bindToController : true,
        replace: true,
        scope : {
          archWidth : '@',
          dimension : '@',
          value : '@',
          label : '@',
          color : '@'
        }
      };
    });

})(window, window.angular);
