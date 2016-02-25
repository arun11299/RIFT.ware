/**
 * Utility functions used in this module
 */

"use strict";
rw.cfg = {

  /**
   * Use schema to find a propery on an object
   */
  propertyValue: function (parent, value, property) {
    var prefix = '';
    if (property.prefix != parent.prefix) {
      prefix = property.prefix + ':';
    }
    return typeof(value) == 'undefined' ? '' : value[prefix + property.name];
  },

  /**
   * Given an item (e.g. search results) find equivalent item on a parent 
   */
  findItem : function (parent, item) {
    var match = _.find(parent.children, function(child) {
      // won't work for list-item
      if (child.type === 'list-item') {
        return (child.value == item.value);
      }
      return (child.property == item.property);
    });
    return match;
  },

  /**
   * REST path useful for sending PUT request to on save
   */
  confdPath : function(item) {
    if (item.parent) {
      var parent = item.parent;
      // recursive
      var path = rw.cfg.confdPath(parent) + '/';
      if (item.type == 'list-item') {
        var keyValue = item.value[item.property.key];
        if (typeof(keyValue) == 'undefined') {
          console.log('Cannot get parent key from', item);
        }
        path += rw.api.encodeUrlParam(keyValue);
      } else {
        path += item.property.name
      }
      return path;
    }
    return  '';
  },

  itemLabel : function(value, type, property) {
    switch (type) {
      case 'list-item':
        return value[property.key];
      case 'list':
        var len = typeof(value) === 'undefined' ? '0' : value.length;
        return property.name + '[' + len + ']';
      default:
        return property.name;
    }
  }
};


/**
 * Navigate configuration and schema trees simultaneously. Delegate operations
 * on items to a given visitor callback. Part of Visitor design pattern.
 */
rw.cfg.Browser = function(config, schema) {
  this.config = config;
  this.schema = schema;
  return this;
}

rw.cfg.Browser.prototype = {

  walk: function(visitor) {
    this.currentPath = [];
    this.walkNode(this.config, this.schema.type, this.schema, visitor);
  },

  walkNode:function(value, type, property, visitor) {
    var self = this;
    var parent = this.currentPath.length > 0 ? this.currentPath[this.currentPath.length - 1] : null;
    this.currentPath.push({property: property, type: type, value: value, parent: parent});
    if (visitor(value, type, property, self)) {
      if (type == 'list') {
        // for lists we treat list items an node members and we invent a new
        // node type for these : list-item
        _.each(value, function (listItem) {
          self.walkNode(listItem, 'list-item', property, visitor);
        });
      } else {
        _.each(property.properties, function(childProperty) {
          var childValue = rw.cfg.propertyValue(property, value, childProperty);
          self.walkNode(childValue, childProperty.type, childProperty, visitor);
        });
      }
    }
    this.currentPath.pop();
  },

  /**
   * Path can act like a key to navigate back to a point in the tree when used
   * with the ReplayPathVisitor.  It might have other uses too.
   */
  getCurrentPath: function() {
    // make a copy because we update our copy
    return this.currentPath.slice(0);
  }
};

/**
 * Find nodes that match a regex and accumlate them into various buckets. Walks every node.
 */
rw.cfg.SearchVisitor = function(regex) {
  this.results = {
    byValue : [],
    byKey : [],
    byDescription: []
  };
  var self = this;

  /**
   * implements visitor pattern looking for nodes that match pattern
   */
  this.visit = function(value, type, property, browser) {
    var newResultItem = function() {
      var path = browser.getCurrentPath();
      var info = path.reduce(function(s, item) {
        return s + '/' + rw.cfg.itemLabel(item.value, item.type, item.property);
      }, '');
      return {
        label : rw.cfg.itemLabel(value, type, property),
        value : value,
        path : path,
        info : info
      };
    };
    if (type === 'leaf' ) {
      if (typeof(value) !== 'undefined' && regex.test(value)) {
        self.results.byValue.push(newResultItem());
      }
    }
    if (type !== 'list-item') {
      if (regex.test(property.name)) {
        self.results.byKey.push(newResultItem());
      }
    }
    return (type !== 'leaf');
  };

  this.totalResults = function() {
    return this.results.byValue.length + this.results.byKey.length + this.results.byDescription.length;
  };

  return this;
};


// ANGULAR

(function(window, angular){

  "use strict";

  angular.module('configEditor', ['rwHelpers'])

    /**
     * <config-editor> - Orchestrate configuration including storing config items
     * in a series of columns.  Items maintain hierarchy to other items.
     */
    .directive('configEditor', function(configFactory) {
      var controller = function($scope, $element) {
        configFactory.get().done(function(config, schema) {
          $scope.config = config.collection;
          $scope.schema = schema.properties[0];/
          var root = {
            label: 'modules',
            property: $scope.schema,
            value: $scope.config,
            type : 'list-item',
            col: 1,
            parent : null
          };
          root.selectRoot = {children:[root]};
          $scope.expandObject(root);
          $scope.columns = [[root]];
          $scope.selected = [];
          $scope.selectItem(root);
          $scope.$apply();
        });

        $scope.query = "";
        $scope.queryUpdated = function() {
          $scope.showResults = ($scope.query.length > 2);
          if ($scope.showResults) {
            var browser = new rw.cfg.Browser($scope.config, $scope.schema);
            var search = new rw.cfg.SearchVisitor(new RegExp('.*' + $scope.query + '.*'));
            browser.walk(search.visit);
            $scope.results = search.results;
            $scope.nResults = search.totalResults();
          }
        };

        $scope.openSearchResult = function(result) {
          $scope.columns.length = 1;
          var parent = $scope.columns[0][0]; // start at the root : modules
          for (var i = 1; i < result.path.length; i++) {
            var isLeaf = result.path[i].type === 'leaf';
            var isLast = (i === result.path.length - 1 || isLeaf);
            var child = rw.cfg.findItem(parent, result.path[i]);
            if (isLast) {
              var selectable  = (isLeaf ? parent : child);
              $scope.selectItem(selectable);
              $scope.showDetails(selectable.col, selectable);
            } else if (child.type === 'list') {
              $scope.expandList(parent.col, parent, child);
            }
            parent = child;
          }
        };

        $scope.showResults = false;

        $scope.setColumnItems = function(column, items) {
          $scope.columns.length = column;
          $scope.columns.push(items);
          $scope.scrollRight();
        };

        $scope.scrollRight = function() {
          var cols = $($element).find('.cb-cols');
          cols.animate({scrollLeft:cols.width()}, 350);
        };

        $scope.crumbs = [];

        $scope.setColumnLength = function(column) {
          $scope.columns.length = column;
        };

        $scope.showDetails = function(columnIndex, item) {
          $scope.selectItem(item);
          $scope.details = item;
          $scope.setColumnLength(columnIndex);
          $scope.isShowDetails = true;
          $scope.scrollRight();
        };

        $scope.isShowDetails = false;

        $scope.updateBreadCrumbs = function(selectedItem) {
          var crumbs = [];
          var getCrumb = function(item) {
            crumbs.push(item);
            if (item.parent) {
              // recurse
              getCrumb(item.parent);
            }
          };
          getCrumb(selectedItem);
          $scope.crumbs = crumbs.reverse();
        };

        $scope.selectItem = function(item) {
          var selector = function(i) {
            i.selected = (i === item);
            // recurse
            if ('children' in i &&
              i.children.length > 0 &&
              i.children[0].selectRoot === item.selectRoot) {
                _.each(i.children, selector);
            }
          };
          _.each(item.selectRoot.children, selector);
          $scope.updateBreadCrumbs(item);
        };

        $scope.expandList = function(columnIndex, parent, item) {
          var value = rw.cfg.propertyValue(parent.property, parent.value, item.property);
          if (value) {
            item.children = value.map(function(value) {
              return {
                label: rw.cfg.itemLabel(value, 'list-item', item.property),
                property: item.property,
                type: 'list-item',
                value: value,
                parent: item,
                selected : false,
                selectRoot : item,
                col: columnIndex + 1
              };
            });
            _.each(item.children, $scope.expandObject.bind($scope));
            $scope.setColumnItems(columnIndex, item.children);
          } else {
            $scope.setColumnLength(columnIndex);
          }
          $scope.selectItem(item);
          $scope.isShowDetails = false;
        };

        $scope.expandObject = function(item) {
          item.children = item.property.properties
            .filter(function(p) {
              return p.type == 'container' || p.type == 'list';
            })
            .map(function(p) {
              var value = rw.cfg.propertyValue(item.property, item.value, p);
              var child = {
                label: rw.cfg.itemLabel(value, p.type, p),
                property: p,
                value: value,
                type : p.type,
                parent: item,
                selected : false,
                selectRoot: item.selectRoot,
                col : item.col
              };
              // recurse
              $scope.expandObject(child);
              return child;
            }
          );
        };
      };
      return {
        restrict: 'E',
        templateUrl:'/integration/_components/config/editor.html',
        controller : controller,
        replace: true
      };
    })

    /**
     * <config-tree> - Recursive tag that renders the tree properties on each item
     * in the config-editor's column items
     */
    .directive('configTree', function(RecursionHelper) {
      var controller = function($scope) {
        $scope.showDetails = $scope.editor.showDetails;
        $scope.expandList = function(columnIndex, childItem) {
          $scope.editor.expandList(columnIndex, $scope.item, childItem);
        };
      };
      return {
        restrict: 'E',
        templateUrl:'/integration/_components/config/tree.html',
        controller : controller,
        replace: true,
        scope: {
          item : '=',
          column: '=',
          editor: '='
        },
        compile: function(element) {
          // Use the compile function from the RecursionHelper,
          // And return the linking function(s) which it returns
          return RecursionHelper.compile(element);
        }
      };
    })

    /**
     * <config-details> - Form editor for leafs and leaf lists
     */
    .directive('configDetails', function() {
      var controller = function($scope) {
        $scope.$watch('item', function() {
          $scope.message = '';
          if (typeof($scope.item) == 'undefined') {
            return;
          }
          $scope.leafs = $scope.item.property.properties
            .filter(function(property) {
              return property.type.indexOf('leaf') === 0;
            })
            .map(function(property) {
              var leaf = _.clone(property);
              leaf.value = rw.cfg.propertyValue($scope.item.property, $scope.item.value, leaf);
              return leaf;
            }
          );
        });

        $scope.save = function() {
          var url = '/api/running' + rw.cfg.confdPath($scope.item);
          var request = {};
          var data = request[$scope.item.property.name] = {};
          $scope.leafs.forEach(function(leaf) {
            data[leaf.name] = parseInt(leaf.value)
          });
          rw.api.put(url, request, 'application/vnd.yang.data+json').done(function() {
            $scope.message = 'Saved';
            $scope.$apply();
          });
        };
      };
      return {
        restrict: 'E',
        templateUrl:'/integration/_components/config/details.html',
        controller : controller,
        replace: true,
        scope: {
          item : '='
        }
      }
    })
})(window, window.angular);
