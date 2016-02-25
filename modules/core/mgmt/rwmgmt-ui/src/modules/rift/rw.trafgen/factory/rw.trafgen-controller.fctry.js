module.exports =(function(window, angular) {


 angular.module('trafgen')
  .factory('trafgenControllerFactory', ['radio', '$rootScope' , function(radio, $rootScope) {
    window.rootscope = $rootScope;

    var factory;

$rootScope.$watch(function(){
      return rw.trafgen.startedPerceived;
    }, function(){
      console.log('startedPerceived updated');
      factory.startedChanged();
    })
    return factory;
  }]);

})(window, window.angular);
