module.exports = angular.module('cmdcntr')
.factory('vcsFactory', ['radio', '$rootScope',
    function(radio, $rootScope) {
        var self = this;
        var appChannel = radio.channel('appChannel');
        return {
            sector: null,
            promise: null,
            attached: function() {
                var self = this;

                if (this.promise) {
                    return self.promise
                };

                var deferred = jQuery.Deferred();

                var getVcs = rw.api.json('/vcs/');
                getVcs.done(function(data) {
                    self.sector = data.collection[0];
                    _.extend(self.sector, rw.vcs);
                    appChannel.trigger('vcs-update');
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
