angular.module('launchPad')


  .directive('createStepLocation', function(createEnvironmentAPI, createStepService, getSearchParams) {
    return {
      restrict: 'AE',
      templateUrl: (getSearchParams.tmpl_url || 'modules/views') + '/rw.launchpad-create_step_location.tmpl.html',
      controllerAs: 'build',
      controller: function(){
        var self = this;

      },
      replace:true,
      link:function(s,e,a) {
        //active, disabled, hide
        angular.element(e).append('<li class="active">Choose</li>');
        createStepService.sub('list-update', function(data){
          angular.element(e).context.querySelector('.active').className = "disabled";
          //e.querySelectorAll('.active').className = "disabled";
          angular.element(e).append('<li class="active">' + data + '</li>');
        })
      }
    }
  })