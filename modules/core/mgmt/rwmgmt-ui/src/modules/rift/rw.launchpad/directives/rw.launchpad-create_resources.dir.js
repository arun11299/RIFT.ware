angular.module('launchPad')
.directive('resourcesTemplate', function(createEnvironmentAPI, createStepService, getSearchParams) {
  "use strict";
  return {
    restrict: 'AE',
    templateUrl: (getSearchParams.tmpl_url || 'modules/views') + '/rw.launchpad-create_resources.tmpl.html',
    controller: function($scope){

      var self = this;

      // Set Subscriptions
      var templatesSub = createEnvironmentAPI.sub('template-resources-update', function(data){
        console.log(data)
        self.resources = data;
      });

      self.environments = createEnvironmentAPI.getEnvironment();
      self.resources = templatesSub.value;
      self.next = function(){
        createStepService.next('Image', function(){
          createEnvironmentAPI.setResources(self.selectedResources, self.lead)
        });
      }



    },
    controllerAs: 'pick'
  };
})