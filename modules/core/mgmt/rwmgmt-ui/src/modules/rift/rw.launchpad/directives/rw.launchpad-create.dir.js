//module.exports = (function(window, angular, rw, $,  undefined) {
//  "use strict";
//  return
  angular.module('launchPad')

    // Primary directive for display form sections.
    // Uses createStepService to determine what the next step should be
    .directive('launchpadCreate', function(createStepService, $compile, $http) {
      "use strict";
      //Steps should be based on template selected
      return {
        restrict: 'AE',
        template: '',
        link: function(s,e,a) {
          // UpdateForm with whatever the current update-list value is.
          // Subscribe to subsequent updates;

          updateForm(
            createStepService.sub('list-update', function(data){
              updateForm(data);
            }).value
          );

          /**
           *
           * @param data string Most recently added step.
           */
          function updateForm(data) {
            e.append($compile(createStepService.key[data])(s));
          }
        }

      };
    })




