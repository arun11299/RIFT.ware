  angular.module('uesimModule')

    .filter('checkDefaultFilter', function () {
      return function (input) {
        if (typeof(input) === 'undefined') {
          return '…';
        }
        return input;
      };
    });

