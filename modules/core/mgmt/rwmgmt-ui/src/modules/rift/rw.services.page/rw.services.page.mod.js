(function(window, angular) {
  angular.module('rwServicesPage',['uiModule','cmdcntr', 'ui.router'])
    .config(function($stateProvider) {
            // SERVICE / VNF
      window.$rw.nav.push({
        module: 'rwServicesPage',
        name: "Services",
        icon: "icon-service"
      });
      $stateProvider.state('services', {
        url: '/services',
        template: '<rw-services-page class="viewport__body"></rw-services-page>'
      });


    });
require('./directives/rw.services.page.dir.js')


})(window, window.angular);
