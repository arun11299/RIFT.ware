(function(window, angular, rw, $,  undefined) {
  "use strict";
  angular.module('ue-sim')
    .directive('uesimulator', function(){
      return{
        restrict: 'C',
        scope: false,
        controller:function($scope){

        },
        controllerAs: 'uesim',
        link: function (scope, element, attr) {

            //
          $("#uetemplatesLyt").mouseIntent(f_slideOut);
          function f_slideOut(){
            $("#uesimulatorLyt").addClass("move");
          }
          $("#uetemplateLyt").mouseIntent(f_slideIn);
          function f_slideIn(){
            $("#uesimulatorLyt").removeClass("move");
          }
        }
      };
    })


})(window, window.angular, window.rw, $);
