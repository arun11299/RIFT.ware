(function(window, angular) {
  angular.module('trafsim')
    .directive('rwTrafsimController', function(radio) {
      return {
        restrict: 'AE',
        controller: controllerFn,
        controllerAs: 'trafsimController',
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

      return rw.trafsim.startedPerceived
    },
    function(){
      self.startedChanged();
    })
    $scope.$watch(function() {
      return rw.trafsim.ratePerceived
    },
    function(){
      self.debouncedRateChanged();
    })

  };

  angular.extend(controllerFn.prototype, {
    created: function() {
      this.g = rw.trafsim;
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
          var command = this.g.startedPerceived ? 'trafsim-resume' : 'trafsim-pause';
          var url = '/api/operations/' + command;
          var meta = {
            input: {
              colony : {
                name : this.colony,
                'trafsim-service' : {
                  name: this.service.name
                }
              }
            }
          };
          rw.api.rpc(url, meta).done(function() {
            // setting rate on trafsim when it's off is lost so we blindly send
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
              'trafsim-service' : {
                'name' : this.service.name,
                'call-rate' : Math.round(this.g.ratePerceived)
              }
            }
          }
        };
        rw.api.rpc('/api/operational/trafsim-adjust', meta);
      },

      detached:function() {
        this._propertyObserver.disconnect_();
      }

  });
})(window, window.angular);
