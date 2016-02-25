module.exports = angular.module('cmdcntr')
.factory('vnfFactory', ['radio', '$rootScope',
    function(radio, $rootScope) {
        var appChannel = radio.channel('appChannel');
        return {
            services: [],
            promise: null,
            attached: function(refresh) {
                var self = this;

                if (this.promise && !refresh) {
                    return this.promise;
                };

                var deferred = jQuery.Deferred();
                var getVnfs = rw.api.json('/vnf/');
                getVnfs.done(function(data) {
                    rw.inplaceUpdate(self.services, data.vnfs.vnf);
                    appChannel.trigger('vnf-update');
                    deferred.resolve();
                });

                self.promise = deferred.promise();
                return self.promise;
            },
            detached: function() {
            }
        };
    }
]);
