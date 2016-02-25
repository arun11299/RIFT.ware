angular.module('launchPad')
  .directive('imageTemplate', function(createEnvironmentAPI, createStepService, getSearchParams) {
    return {
      restrict: 'AE',
      templateUrl: (getSearchParams.tmpl_url || 'modules/views') + '/rw.launchpad-create_image.tmpl.html',
      controllerAs: 'app',
      controller: function(){
        var self = this;
        var imagesSub = createEnvironmentAPI.sub('images-update', function(data){
          self.images = data.images;
        });

        self.environment = createEnvironmentAPI.getEnvironment();
        self.images = imagesSub.value;
        self.next = next;
        function next() {
          var nextStep = self.environment.fields ? 'Config' : 'Confirm';
          createStepService.next(nextStep, function() {
            createEnvironmentAPI.updateEnvironment(self.environment);
          });
        }
      }
    }
  })