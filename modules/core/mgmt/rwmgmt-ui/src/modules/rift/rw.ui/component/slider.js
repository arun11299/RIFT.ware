// Q: How do i test this?
// Q: Is there a useful way to note js and css dependencies
// Q: How do i jsdocument this?
(function(window, angular) {

  angular.module('uiModule')
    .directive('rwSlider', function() {
      var controller = function($scope, $element, $timeout) {
        // Q: is there a way to force attributes to be ints?
        $scope.min = $scope.min || "0";
        $scope.max = $scope.max || "100";
        $scope.step = $scope.step || "1";

        $($element).noUiSlider({
          start: parseInt($scope.value),
          step: parseInt($scope.step),
          range: {
            min: parseInt($scope.min),
            max: parseInt($scope.max)
          }
        });
        var onSlide = function(e, value) {
          $timeout(function(){
            $scope.value = value;
          })

        };
        $($element).on({
          change: onSlide,
          slide: onSlide
        });

        $scope.$watch('value', function(value) {
          $($element).val(value);
        });
      };

      return {
        restrict : 'E',
        template: '<div></div>',
        controller : controller,
        replace: true,
        scope: {
          min : '@',
          max : '@',
          step : '@',
          value : '=' // two-way binding
        }
      };
    });

})(window, window.angular);

