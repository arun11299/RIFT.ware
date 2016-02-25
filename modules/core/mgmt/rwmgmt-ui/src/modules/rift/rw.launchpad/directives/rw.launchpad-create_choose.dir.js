angular.module('launchPad')


  .directive('chooseTemplate', function(createEnvironmentAPI, createStepService, getSearchParams) {
    "use strict";
    return {
      restrict: 'AE',
      templateUrl: (getSearchParams.tmpl_url || 'modules/views') + '/rw.launchpad-create_choose.tmpl.html',
      controller: function($scope){
        var self = this;
        // Set Subscriptions
        var templatesSub = createEnvironmentAPI.sub('templates-update', function(data){
          self.templates = data.templates;
        });

        self.next = createStepService.next;
        self.selectedTemplate = '';
        self.setTemplate = createEnvironmentAPI.setTemplate;
        // Binds templates data to existing value, if there is one.


        // Init the template list
        createEnvironmentAPI.getTemplates();


      },
      controllerAs: 'choose'
    };
  })