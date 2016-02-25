angular.module('launchPad')
  .directive('configTemplate', function(createEnvironmentAPI, createStepService, getSearchParams) {
    return {
      restrict: 'AE',
      templateUrl: (getSearchParams.tmpl_url || 'modules/views') + '/rw.launchpad-create_config.tmpl.htmll',
      controllerAs: 'config',
      controller: function(){
        // Init
        createEnvironmentAPI.getConfig();
        var nextStep = 'Confirm';
        var self = this;
        var configSub = createEnvironmentAPI.sub('config-update', function(data){
          self.config = data;
        });

        self.environment = createEnvironmentAPI.getEnvironment();
        self.config = configSub.value;
        self.next = next;
        function next() {
          //var nextStep = self.environment.fields ? 'Config' : 'Confirm';
          createStepService.next(nextStep, function() {
            createEnvironmentAPI.updateConfig(self.config)
            createEnvironmentAPI.updateEnvironment(self.environment);
          });
        }
      }
    }
  })