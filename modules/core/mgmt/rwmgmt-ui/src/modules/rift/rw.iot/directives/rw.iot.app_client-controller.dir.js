(function(window, angular) {
  angular.module('rw.iot')
    .directive('iotController', function(radio) {
      return {
        restrict: 'AE',
        controller: controllerFn,
        controllerAs: 'iotController',
        bindToController: true,
        scope: {
          service: '=?',
          colony: '=?'
        }
      }
    });

  var controllerFn = function($scope, radio) {
    var self = this;
    self.created();
    $scope.$watch(function() {

      return rw.iot.startedPerceived
    },
    function(){
      self.startedChanged();
    })
    $scope.$watch(function() {
      return rw.iot.ratePerceived
    },
    function(){
      self.debouncedRateChanged();
    })

  };

  angular.extend(controllerFn.prototype, {
    created: function() {
      this.g = rw.iot;
        if (this.g.startedActual !== null) {
          this.g.startedPerceived = this.g.startedActual;
        }
        if (this.g.rateActual !== null) {
          this.g.ratePerceived = this.g.rateActual;
        }
        // RIFT-5794 - Too many quick changes cause thrashing on backend
        this.debouncedRateChanged = _.debounce(this.rateChanged.bind(this), 200);
      },

      startedChanged: function() {
        var self = this;
        var change = this.g.startedActual !== null && this.g.startedActual != this.g.startedPerceived;
        if (this.colony && this.service && change) {
          var command = this.g.startedPerceived ? 'iot-resume' : 'iot-pause';
          var url = '/api/operations/' + command;
          var meta = {
            input: {
              colony : {
                name : this.colony,
                'iot-service' : {
                  name: this.service.name
                }
              }
            }
          };
          rw.api.rpc(url, meta).done(function() {
            // setting rate on iot when it's off is lost so we blindly send
            // rate immediately after a start
            if (self.g.startedPerceived) {
              self.sendRate();
            }
          });
        }
      },

      rateChanged: function() {
        if (this.g.rateActual !== null && this.colony && this.service) {
          this.sendRate();
        }
      },

      sendRate: function() {
        var meta = {
          input: {
            colony : {
              name : this.colony,
              'iot-service' : {
                'name' : this.service.name,
                'call-rate' : Math.round(this.g.ratePerceived)
              }
            }
          }
        };
        rw.api.rpc('/api/operational/iot-adjust', meta);
      },

      detached:function() {
        this._propertyObserver.disconnect_();
      }

  });
})(window, window.angular);
