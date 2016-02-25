(function(window, angular) {
  angular.module('uiModule')
    .directive('rwGeomap', function() {
      return {
        restrict:'AE',
        controller: controller,
        controllerAs: 'geomapController',
        replace: true,
        template: '<div class="rw-geomap"></div>',
        scope: {
          markers: '='
        }
      }
    });
  var controller = function($scope, $element){
    var self = this;
    self.attached($scope, $element);
    $scope.$watch(function() {
      return $scope.markers;
    }, function() {
      self.markersChanged();
    });

  };

  controller.$inject = ['$scope', '$element']
  angular.extend(controller.prototype, {
    attached: function(scope, element) {
      this.scope = scope;
      this.element = element;
      this.map = new jvm.WorldMap({
        container: $(this.element),
        map: 'world_mill_en',
        markerStyle : {
          initial: {
            fill: 'rgba(73, 189, 238, 1)',
            stroke: '#1f80ae',
            "fill-opacity": 1,
            "stroke-width": 1,
            "stroke-opacity": 1,
            r: 10
          },
          hover: {
            stroke: 'black',
            "stroke-width": 2
          },
          selected: {
            fill: 'blue'
          },
          selectedHover: {
          }
        },
        backgroundColor: "#3b3b3b",
        regionsSelectable: false,
        markersSelectable: true,
        markersSelectableOne: true,
        onMarkerSelected: this.markerSelected.bind(this)
      });
    },

    markerSelected: function(marker) {
      console.log('selected', marker);
    },

    markersChanged: function() {
      this.map.addMarkers(this.scope.markers);
    },

    setSelected: function(item) {
      if (!item.marker) {
        console.log('item does not have a marker', item);
      } else {
        this.map.clearSelectedMarkers();
        var ndx = this.scope.markers.indexOf(item.marker);
        this.map.setSelectedMarkers(['' + ndx]);
      }
    }
  });
})(window, window.angular);
