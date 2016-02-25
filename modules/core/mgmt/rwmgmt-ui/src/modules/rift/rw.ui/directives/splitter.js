(function(window, angular) {
  angular.module('uiModule')
    .directive('splitter', function() {
      return {
        restrict:'AE',
        transclude: true,
        controller: controller,
        controllerAs: 'splitterController',
        replace: true,
        templateUrl: '/modules/views/rw.splitter.tmpl.html',
        scope: {
          markers: '='
        }
      }
    });
  var controller = function($scope, $element){
    var $handle = $element;
    //var target = document.getElementById('bottom');
    var size;
    //$handle.prev().css('overflow', 'hidden');

    //$handle.next().css('overflow', 'scroll');
    var mY = 0;
    $scope.prevHeight = "0px";
    $scope.iconUp = false;
    $scope.collapse = function() {
      var next = $handle.next()[0];
      if (next.style.height != "0px") {
        $scope.prevHeight = next.style.height;
        next.style.height = "0px";
        $scope.iconUp = true;
      } else {
        next.style.height = $scope.prevHeight;
        $scope.iconUp = false;
      }
    };
    $scope.expand = function() {
      var next = $handle.next()[0];
      var body = document.body,
        html = document.documentElement;
      next.style.height = Math.max( body.scrollHeight, body.offsetHeight,
        html.clientHeight, html.scrollHeight, html.offsetHeight ) + "px";
    }
    $scope.dragSplitter = function($event) {
      var e = $event;
      //console.log(e);
      var body = document.body,
        html = document.documentElement;
      var size  = Math.max( body.scrollHeight, body.offsetHeight,
        html.clientHeight, html.scrollHeight, html.offsetHeight ); //parseInt(getComputedStyle(window).height);
      console.log( body.scrollHeight, body.offsetHeight,
        html.clientHeight, html.scrollHeight, html.offsetHeight)
      var d = e.pageY;
      //console.log(e.target);
      //console.log(e.target.parentElement.nextElementSibling);
      // var prev = e.target.parentElement.previousElementSibling;
      // var next = e.target.parentElement.nextElementSibling;

      var prev = $handle.prev()[0];
      var next = $handle.next()[0];
      //target.style.height = size - d + 'px';

      var handlers = {
        mousedown: function(e) {
          e.preventDefault();
        },
        mousemove: function(e) {
          var d = e.pageY;
          //40 IS A MAGIC NUMBER. IT IS THE HEIGHT OF THE HEADER. LOOKING INTO A BETTER WAY TO DO THIS
          var offset = ($('#handle').height() / 2);
          //console.log(offset);
          console.log('size', size, 'pagey ', e.pageY, 'offset', offset)
          next.style.height = (size - (e.pageY + offset)) + "px";
          // prev.style.height = (e.pageY - offset) + 'px';
          //console.log((size - (e.pageY + 20)) + "px");
          //console.log(target.style.height);

          //size + ((e.pageY < mY) ? -d : d) + 'px';
          mY = e.pageY;
        },
        mouseup: function() {
          $(this).off(handlers);
          //console.log(next.style.height);

        }
      };
      $(document).on(handlers);
    };

  };
})(window, window.angular);
