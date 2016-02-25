(function(window, angular) {
  angular.module('configuration')
    .directive('rwConfigViewer', function(RecursionHelper) {
      return {
        restrict:'AE',
        controller: controller,
        controllerAs: 'configViewerController',
        templateUrl: '../../modules/views/rw.config-viewer.tmpl.html',
        scope: {
          config: '=',
          label: '@'
        },
        compile: function(element) {
          return RecursionHelper.compile(element);

        }
      }
    });
  var controller = function($scope, $element) {


    $scope.created = function (scope, element) {
      $scope.expanded = false;
      //$scope.configChanged();
    };




    /**
     * @method expandAll(boolean)
     * Deeply expand or collapse this node and ALL children.
     *
     * known issue: doesn't do deep expand if expandAll is called
     * in attach life cycle presumably because children's
     * rw-config-viewer cannot be found in shadowroot
     */
    $scope.expandAll = function (expand, id) {
      //console.log("!!!!!!")
      //console.log($scope);
      //console.log($scope.$id, id.id);
      if ($scope.$id != id.id) {
        $scope.expanded = expand;
        $scope.expandedChanged();
      }
      //var childViewers = $scope.shadowRoot.querySelectorAll('rw-config-viewer');
      //[].forEach.call(childViewers, function (el, i) {
      //  el.expandAll(expand);
      //});
      //$scope.$broadcast("expandAll", $scope.$id);
    };

    $scope._toggleExpanded = function (e) {
      //console.log(e);
      e.stopPropagation();
      $scope.expanded = !$scope.expanded;
      if ($scope.expanded) {
        $scope.$broadcast('expandOne', {id:$scope.$id});
      } else {
        $scope.$broadcast('collapseAll', {id:$scope.$id});
      }
      //$scope.$apply();
      $scope.expandedChanged();

    };



    $scope.expandedChanged = function () {

      //console.log('expandedChanged', $scope.expanded);
      $scope.expandStyle = $scope.expanded ? 'block' : 'none';
      $scope.expandIconClass = $scope.expanded ? 'node-open' : 'node-closed';
      //console.log($scope.expandStyle);

    };


    $scope.refresh = function (id) {
      //console.log($scope.$id, id.id)
      if ($scope.$id != id.id) {
        $scope.configChanged();
        //$scope.$broadcast('refresh', {id:$scope.$id});

      }
      //var children = $scope.shadowRoot.querySelectorAll('rw-config-viewer');
      //for (var i = 0; i < children.length; i++) {
      //  children[i].refresh();
      //}
    };


    $scope.configChanged = function () {
      $scope.isArray = Array.isArray($scope.config);
      $scope.isObject = $scope.config instanceof Object && !$scope.isArray;
      $scope.keys = $scope.isObject ? _.keys($scope.config) : null;
      $scope.keys_modified = [];
      for (var i = 0; $scope.keys != null && i < $scope.keys.length; i++) {
        if ($scope.keys[i] != "$$hashKey") {
          $scope.keys_modified.push($scope.keys[i]);
        }
      }
      $scope.isPrimative = !($scope.config instanceof Object);
      $scope.isEmpty = ($scope.isArray ? $scope.config.length == 0 :
        ($scope.isObject ? _.isEmpty($scope.config) : false));
    };
    //console.log('define');
    var self = $scope;
    //$scope.expanded = false;

    self.created($scope, $element);
    $scope.$watch('config', $scope.configChanged());
    $scope.$watch('expanded', $scope.expandedChanged());
    $scope.$on('expandOne', function(e,id) {
      if (event.defaultPrevented) {
        return
      }
      event.preventDefault();

      //console.log('expand recieved')
      $scope.expandAll(true, id);
    })
    $scope.$on('expandAll', function(e, id) {
      //console.log('expand recieved')
      $scope.expandAll(true, id);
    })
    $scope.$on('collapseAll', function(e, id) {
      //console.log('collapse recieved')
      $scope.expandAll(false, id);
    })

    $scope.$on('refresh', function (e, id) {
      //console.log('refresh recieved');
      $scope.refresh(id)
    });
    controller.$inject = ['$scope', '$element']
  };
})(window, window.angular);
