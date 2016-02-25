angular.module('launchPad')


  .directive('buildTemplate', function(createEnvironmentAPI, createStepService, getSearchParams) {
    return {
      restrict: 'AE',
      templateUrl: (getSearchParams.tmpl_url || 'modules/views') + '/rw.launchpad-create_build.tmpl.html',
      controllerAs: 'build',
      controller: function(){
        var self = this;
        var poolsWatcher = createEnvironmentAPI.sub('pools-update', function(data){
          self.pools = data;
        });

        self.environment = createEnvironmentAPI.getEnvironment();
        self.next = next;
        self.pools = poolsWatcher.value;

        function next() {
          createStepService.next('Resources', function() {
            createEnvironmentAPI.updateEnvironment(self.environment);
          });
        }
      }
    }
  })