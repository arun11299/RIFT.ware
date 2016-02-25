//(function(window, angular){
//  var base;
//  try{
//    base = angular.module('core')
//  } catch (e){
//    base = angular.module('core',[]);
//  }
//
//  base.factory('navData', function(){
//    var f = {};
//    var setOrder = function(nav, modules){
//      var ordered = 0;
//      console.log('sorting', modules)
//      return nav.sort(function(a,b){
//        return modules.indexOf(a.module) - modules.indexOf(b.module);
//      })
//
//    };
//    f.get = function(cb){
//      var nav = setOrder(window.$rw.nav, angular.module('modules').requires);
//      return cb(nav);
//    };
//  return f;
//  })
//})(window, window.angular);