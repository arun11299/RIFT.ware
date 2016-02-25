angular.module('services',['ngResource'])
    .factory('vnfData',function($resource){
        return $resource(rw.api.server + '/vnf/');
    })
    .factory('trafgenData',function($resource){
        return $resource('/tragen/:type/:command/:colony_id/:port',
            {
                type:'@type',
                command:'@command',
                colony_id:'@colony_id',
                port:'@port'
            });
    })
    .controller('trafgenCtrl',function($scope,trafgenData){

        $scope.toggleStart = function(){

        };

    })