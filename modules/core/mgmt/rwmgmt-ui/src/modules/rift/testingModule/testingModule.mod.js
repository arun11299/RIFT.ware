(function(window, angular){
  "use strict";

  angular.module('testingModule',['ngMockE2E']).run(function($httpBackend){
    console.log('running testingModule')
    $httpBackend.whenGET(/\.html$/).passThrough();
    $httpBackend.whenGET(/[.+]?vnf/).respond(function(){
      return [200, {'test':'yeah'}, {}];
    });
    $httpBackend.whenGET('http://localhost:5050/vnf').respond(function(){
      return [200, {'test':'yeah2'}, {}];
    });
    console.log('complete testing module')
  })
})(window, window.angular);