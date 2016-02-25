(function (window, angular, rw, undefined) {


  angular.module('uptime',['ngResource'])
    .directive("uptime",function($interval){

      return {
        restrict:'AE',
        template:'<span><span class="icn fa-minus-circle" ng-if="toString() == \'Inactive\'"></span>{{toString()}} </span>',
        controllerAs: 'uptime',
        scope:{
          initialtime:'=',
          run:'='
        },
        controller:function($scope) {
        },
        link: function($scope, element, attr) {
          var tick;
          var running = false;

          $scope.$watch(
            function() {
              return $scope.initialtime;
            },
            function() {
              if ($scope.run) {
                $interval.cancel(tick);
                tick = $interval(updateTime, 1000);
              }
              if (typeof($scope.initialtime) === 'undefined') {
                $scope.time = {days:0, hours:0, minutes:0, seconds:0}
              } else {
                $scope.time = $scope.convert($scope.initialtime);
              }

            });

          $scope.$watch(
            function() {
              return $scope.run;
            },
            function() {
              if($scope.run) {
              } else {
                $interval.cancel(tick);
              }
            }
          )

          $scope.convert = function(input) {
            ret = {days:0, hours:0, minutes:0, seconds:0};
            if (input == "Inactive" || typeof(input) === 'undefined') {
              ret.seconds = -1;
            } else if (input != "" && input != "Expired") {
              ret.seconds = input % 60;
              ret.minutes = Math.floor(input / 60) % 60;
              ret.hours = Math.floor(input/3600) % 24;
              ret.days = Math.floor(input/86400);
            }
            return ret;
          }

          //$scope.convert = function(str) {
          //  ret = {days: 0, hours: 0, minutes: 0, seconds: 0};
          //  if (str == "Inactive") {
          //    ret.seconds = -1;
          //  } else if (str != "" && str != 'Expired') {
          //    arr = str.split(':');
          //    arr.forEach(function (ele, i) {
          //      unit = ele[ele.length - 1].toLowerCase();
          //      value = ele.substring(0, ele.length - 1);
          //      if (unit === "d") {
          //        ret.days = parseInt(value);
          //      } else if (unit === "h") {
          //        ret.hours = parseInt(value);
          //      } else if (unit === "m") {
          //        ret.minutes = parseInt(value);
          //      } else if (unit === "s") {
          //        ret.seconds = parseInt(value);
          //      }
          //    });
          //  }
          //  return ret;
          //};
          $scope.toString = function() {
            if (!$scope.time) {
              return;
            }
            var ret = "";
            if ($scope.time.days > 0) {
              ret +=  $scope.time.days + "d:";
            }
            if ($scope.time.hours > 0) {
              ret +=  $scope.time.hours + "h:";
            }
            if ($scope.time.minutes > 0) {
              ret +=  $scope.time.minutes + "m:";
            }
            if ($scope.time.seconds > 0) {
              ret += $scope.time.seconds +  "s";
            }
            if (ret == "") {
              return "--";
            }
            if ($scope.time.seconds == 0 && ret != "") {
              ret = ret.substring(0, ret.length - 1);
            }
            return ret;
          };

          function updateTime() {
            if ($scope.time.seconds == -1) {
              return;
            }
            if ($scope.time.seconds != 59) {
              $scope.time.seconds++;
            } else if ($scope.time.minutes != 59) {
              $scope.time.seconds = 0;
              $scope.time.minutes++;
            } else if ($scope.time.hours != 23) {
              $scope.time.seconds = 0;
              $scope.time.minutes = 0;
              $scope.time.hours++;
            } else  {
              $scope.time.seconds = 0;
              $scope.time.minutes = 0;
              $scope.time.hours = 0;
              $scope.time.days++;
            }
          }

        }
      }
    })
})(window, window.angular, window.rw);