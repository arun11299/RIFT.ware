// Q: How do i test this?
// Q: Is there a useful way to note js and css dependencies
// Q: How do i jsdocument this?
(function(window, angular) {

  /**
   * <rw-bullet>
   */
  var controller = function($scope, $element) {
    this.$element = $element;
    this.vertical = false;
    this.value = 0;
    this.min = 0;
    this.max = 100;
    this.bulletColor = "blue";
    this.radius = 4;
    this.containerMarginX = 0;
    this.containerMarginY = 0;
    this.bulletMargin = 0;
    this.width = 512;
    this.height = 64;
    this.markerX = -100; // puts it off screen unless set
    var self = this;
    $scope.$watch(
      function() {
        return self.value;
      },
      function() {
        self.valueChanged();
      }
    );

  };

  controller.prototype = {

    valueChanged: function() {
      var range = this.max - this.min;
      var normalizedValue = (this.value - this.min) / range;
      // All versions of IE as of Jan 2015 does not support inline CSS transforms on SVG
      if (platform.name == 'IE') {
        this.bulletWidth = Math.round(100 * normalizedValue) + '%';
      } else {
        this.bulletWidth = this.width - (2 * this.containerMarginX);
        var transform = 'scaleX(' + normalizedValue + ')';
        var bullet = $(this.$element).find('.bullet2');
        bullet.css('transform', transform);
        bullet.css('-webkit-transform', transform);
      }
    },

    markerChanged: function() {
      var range = this.max - this.min;
      var w = this.width - (2 * this.containerMarginX);
      this.markerX = this.containerMarginX + ((this.marker - this.min) / range ) * w;
      this.markerY1 = 7;
      this.markerY2 = this.width - 7;
    }
  };

  angular.module('uiModule')
    .directive('rwBullet2', function() {
      return {
        restrict : 'E',
        templateUrl: '/modules/views/rw.bullet2.tmpl.html',
        controller : controller,
        bindToController: true,
        controllerAs: 'bullet',
        replace: true,
        scope: {
          min : '@?',
          max : '@?',
          value : '@',
          marker: '@?',
          bulletColor: '@?'
        }
      };
    });

})(window, window.angular);
