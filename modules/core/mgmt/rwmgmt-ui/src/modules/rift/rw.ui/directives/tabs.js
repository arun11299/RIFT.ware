/**
 * Command Center Module
 * @fileoverview Command Center Module parent
 */
(function(window, angular) {
  "use strict";

  var tabsCntl = function($scope) {
    var panes = $scope.panes = [];

    $scope.select = function(pane) {
      angular.forEach(panes, function(pane) {
        pane.selected = false;
      });
      pane.selected = true;
      console.log(pane);
    }

    this.addPane = function(pane) {
      if (panes.length == 0) $scope.select(pane);
      panes.push(pane);
    }
  }
  angular.module('uiModule').
    directive('rwTabs', function() {
      return {
        restrict: 'E',
        controller: tabsCntl,
        transclude: true,
        scope: {},
        template:
        '<div class="tabs-cntr">' +
        '<ul class="tabs-list">' +
        '<li ng-repeat="pane in panes" class="tabs-tab" ng-class="{active:pane.selected}">'+
        '<span href="" ng-click="select(pane)">{{pane.title}}</span>' +
        '</li>' +
        '</ul>' +
        '<div class="tab-content" ng-transclude></div>' +
        '</div>',
        replace: true
      };
    }).
    directive('pane', function() {
      return {
        require: '^rwTabs',
        restrict: 'E',
        transclude: true,
        scope: { title: '@' },
        link: function(scope, element, attrs, tabsCtrl) {
          tabsCtrl.addPane(scope);
        },
        template:
        '<div class="tabs-pane" ng-class="{active: selected}" ng-transclude>' +
        '</div>',
        replace: true
      };
    });

})(window, window.angular);



