(function (window, angular, rw, undefined){
  "use strict";
  angular.module('vnfService',[])
    .factory('vnfService', function($q){
      var self = {};
      self.get = function(){
        return $q(function(resolve, reject){
          rw.api.json('/vnf').done(function(data){
            var vnf = data.vnfs.vnf[0];
            self.type = vnf.type;
            self.ip = vnf.vm[0].vm_info['vm-ip-address'];
            self.name = vnf.vm[0].vm_info.leading;
            resolve(data);
          });
        });
      };
      return self;
    });
})(window, window.angular, window.rw);
