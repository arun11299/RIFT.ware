(function(window, angular) {
  angular.module('configuration')
    .directive('rwConfiguration', function() {
      return {
        restrict: 'AE',
        controller: controller,
        controllerAs: 'configurationController',
        templateUrl: '../../modules/views/rw.configuration.tmpl.html',
        scope: {}
      }
    });
  var controller = function($scope, $element) {
    var self = $scope;

    $scope.created = function() {
      $scope.config = {};

    };
    $scope.attached = function(scope, element) {
      this.scope = scope;
      this.element = element;
      $scope.refresh();
      $scope.refresh();
    };
    $scope.expandAll = function() {
      $scope.$broadcast('expandAll', {
        id: $scope.$id
      })

    };

    $scope.collapseAll = function() {
      $scope.$broadcast('collapseAll', {
        id: $scope.$id
      })
    };

    $scope.refresh = function() {

      var self = $scope;
      console.log('refreshing')
      var url = '/api/running/colony';
      var acceptType = 'application/vnd.yang.collection+json';
      rw.api.get(url, acceptType).done(function(config) {
        self.config = config;
        rw.api.get(url + '?deep', acceptType).done(function(deepConfig) {
          rw.merge(self.config, deepConfig);
          //self.$.configViewer.refresh();
          self.scope.$broadcast('refresh', {
            id: self.$id
          });
          $scope.$apply();
        });
      });
      self.scope.refresh = self.refresh;
    };
    $scope.created();
    angular.element(document).ready(function () {
    $scope.attached($scope, $element);
    });

  };
  controller.$inject = ['$scope', '$element']

})(window, window.angular);
