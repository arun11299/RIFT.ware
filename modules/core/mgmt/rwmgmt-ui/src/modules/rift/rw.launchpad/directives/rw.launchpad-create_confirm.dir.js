angular.module('launchPad')

  .directive('confirmTemplate', function(createEnvironmentAPI, createStepService, getSearchParams) {
    return {
      restrict: 'AE',
      templateUrl: (getSearchParams.tmpl_url || 'modules/views') + '/rw.launchpad-create_confirm.tmpl.html',
      controllerAs: 'confirm',
      controller: function(){
        var self = this;
        //var confirmWatcher = createEnvironmentAPI.sub('confirm-update', function(data){
        //  self.confirm = data;
        //  console.log(data)
        //});

        self.environment = createEnvironmentAPI.getEnvironment(function(){
          //if (!self.environment.lead) {
          //  self.environment.lead = self.environment.resources[0],name
          //}
          //console.log(self.environment)
        });

        self.confirm = createEnvironmentAPI.getConfirmData(self.environment);
        console.log(self.confirm.lead)
        self.resourceCache = createEnvironmentAPI.resourceCache();
        //console.log(self.confirm, self.environment)
        self.create = create;
        function create(launch) {
          createEnvironmentAPI.createEnvironment(self.environment, launch);
        }
        //debugger;
        self.filterSelectedVM = createEnvironmentAPI.filterSelectedVM;
      }
    }
  })