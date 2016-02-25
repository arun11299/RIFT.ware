module.exports = (function(window, angular){
  "use strict";

  /**
   * <vcs-table> -
   */
  var controller = function($scope, vcsFactory, $timeout) {
    var self = this;
    self.created();
    vcsFactory.attached().done(function() {
console.log("sector changed", vcsFactory.sector, "$scope", $scope);
      self.sector = vcsFactory.sector;
      self.sectorChanged();
      // when a user hits dashboard page for first time w/o cache then
      // VCS table does not render.  Otherwise this timeout would not
      // be nec.
      $timeout(function() {
        $scope.$apply();
      });
    });
  };

  controller.prototype = {
    created: function() {
      this.rows = [];
    },

    sectorChanged: function() {
      if (!('collection' in this.sector)) {
        return;
      }
console.log("valid sector");
      this.rows.length = 0;
      this.addRows(this.rows, this.sector, 0);
      this.selectVcs(this.rows[0]);
    },

    addRows: function(rows, node, depth) {
      if (depth > 0) {
        rows.push({node : node, depth: depth});
      }
      var self = this;
      var recurse = function(child) {
        self.addRows(rows, child, depth + 1);
      };
      if (Array.isArray(node.collection)) {
        _.each(node.collection, recurse);
      }
    },

    selectVcs: function(selectedRow) {
      _.each(this.rows, function(row) {
        row.selected = (row === selectedRow);
      });
      if (this.onSelectVcs) {
        this.onSelectVcs({
          node: selectedRow.node
        });
      }
    }
  };

  angular.module('cmdcntr')
    .directive('vcsTable', function() {
      return {
        restrict: 'AE',
        templateUrl: '/modules/views/rw.cmdcntr-vcs_table.tmpl.html',
        controller: controller,
        controllerAs : "vcsTable",
        bindToController : true,
        replace: true,
        scope : {
          onSelectVcs : '&'
        }
      };
    });

})(window, window.angular);
