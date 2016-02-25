module.exports = (function(window, angular) {
  angular.module('trafgen')
    .directive('trafgenController', function(radio) {
    return {
      restrict:'AE',
      controller: controller,
      controllerAs: 'trafgenController',
      bindToController: true,
      scope: {
        service: '='
      }
    };
  });
    var controller = function($scope, radio){
      //There should be a better way of doing this. Circular watchers with rw.trafgen-control.dir.js
    var self = this;
    var firstRun = true;
    var appChannel = radio.channel('appChannel');
    self.appChannel = appChannel;
      self.created();
      $scope.$watch(function() {
        return rw.trafgen.startedPerceived;
      }, function(old, nev) {
        if (nev !== null) {
          self.startedChanged();
        }
      });
      $scope.$watch(function() {
        return rw.trafgen.ratePerceived;
      }, function(oldVal, newVal) {
        if (newVal !== null && oldVal !== newVal && !firstRun) {
          self.debouncedRateChanged();
        }
        firstRun = false;
      });
      $scope.$watch(function() {
        return rw.trafgen.packetSizePerceived;
      }, function(oldVal, newVal) {
        if (newVal !== null && oldVal !== newVal) {
          self.debouncedPacketSizeChanged();
        }
      });
      $scope.$watch(function() {
        return self.service;
      }, function(oldVal, newVal) {
        if (self.service !== null) {
          self.serviceChanged();
        }
      });

  rw.BaseController.call(this);
    }

    controller.$inject = ['$scope', 'radio']
    angular.extend(controller.prototype, {
      observe : {
        'g.startedPerceived': 'startedChanged',
        'g.ratePerceived': 'debouncedRateChanged',
        'g.packetSizePerceived': 'debouncedPacketSizeChanged'
      },

      created: function() {
        this.g = rw.trafgen;

        // RIFT-5794 - Too many quick changes cause thrashing on backend
        this.debouncedRateChanged = _.debounce(this.rateChanged.bind(this), 200);
        this.debouncedPacketSizeChanged = _.debounce(this.packetSizeChanged.bind(this), 200);
      },

      serviceChanged: function() {
        var self = this;
        // missing fabric ports? can they generate traffic too?
        if (!this.service) {
          return false;
        }
        self.ports = jsonPath.eval(self.service, "vm[*].port[*]");
        if (self.ports.length > 0) {
          // all ports for a service should be in same colony
          self.colony = self.ports[0].colonyId;
        }
      },

      startedChanged: function() {
        var change = this.g.startedActual !== null && this.g.startedActual != this.g.startedPerceived;
        if (this.colony && change) {
          var command = this.g.startedPerceived ? 'start' : 'stop';
          var data = {
            input : {
              colony : {
                name : this.colony,
                traffic : {
                  all : ""
                }
              }
            }
          };
          rw.api.rpc('/api/operations/' + command, data);
          // send the rate when starting in case perceived != actual
          if (this.g.startedPerceived) {
            this.sendRateChange();
          }
        }

      },

      rateChanged: function() {
        if (this.g.rateActual !== null) {
          this.sendRateChange();
        }
      },

      sendRateChange: function() {
        this.g.rateDejitterT0 = new Date().getTime();
        var url = '/api/running/colony/' + this.colony + '/port/@PORT@/trafgen/transmit-params';
        this.ajaxEachPort(url, {
            'transmit-params' : {
              'tx-rate' : parseInt(this.g.ratePerceived)
            }
          }
        );
      },

      packetSizeChanged: function() {
        if (this.g.rateActual !== null) {
          this.sendPacketSizeChange();
        }
      },

      sendPacketSizeChange: function() {
        this.g.rateDejitterT0 = new Date().getTime();
        var self = this;
        var url = '/api/running/colony/' + this.colony + '/port/@PORT@/trafgen/range-template/packet-size';
        this.ajaxEachPort(url, {
            'packet-size' : {
              increment: 1,
              start: parseInt(self.g.packetSizePerceived),
              minimum: parseInt(self.g.packetSizePerceived),
              maximum: parseInt(self.g.packetSizePerceived)
            }
          }
        );
      },

      ajaxEachPort: function(url, meta) {
        if(!this.ports){
          this.serviceChanged();
        }
        _.each(this.ports, function(port) {
          var portUrl = url.replace(/@PORT@/g, rw.api.encodeUrlParam(port.name));
          rw.api.put(portUrl, meta, 'application/vnd.yang.data+json');
        });
      },

      detached:function() {
        this._propertyObserver.disconnect_();
      }
    });
})(window, window.angular);
