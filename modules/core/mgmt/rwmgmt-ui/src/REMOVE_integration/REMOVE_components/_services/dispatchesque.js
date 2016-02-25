//Super lightweight event callback dispatcher
(function(window, angular){
  angular.module('dispatchesque',[])
    .factory('dispatcher', function(dispatchesque) {
      return new dispatchesque();
    })
    .factory('dispatchesque',function(){
      var factory = function () {
        var self = this;
        var callbacks = {};
        var currentID = 0;
        self.emit = function(event, value){
          self[event] = value;
          if(callbacks.hasOwnProperty(event)){
            //completed and waiting currently serve no true functional purpose
            //they are currently reminders that the "waitFor" functionality has not yet been implemented
            var completed = [],
              waiting = [];
            for(var k in callbacks[event]){
              if(callbacks[event].hasOwnProperty(k)){
                var c = callbacks[event][k];
                //Need to implement isReady
                if(!c.waitFor || isReady(c, completed)){
                  try{
                    callbacks[event][k].cb(self[event]);
                  } catch (e){
                    console.log('Event callback error: ', e);
                  }
                }
              }
            }
          }
        };
        self.watch = function(event, cb){
          if(!callbacks[event]) callbacks[event] = {};
          callbacks[event][currentID] = {
            cb:cb,
            waitFor:[]
          };
          currentID++;
          return {
            id: currentID,
            value: self[event],
            waitFor: waitFor,
            cancel: cancel.bind(self, event, currentID)
          };
        };

        function cancel(event, id){
          delete callbacks[event][id];
        }
        function isReady(c, d){
          return true;
        }
        function waitFor(ids){

        }

      };
      return factory;
    })
})(window, window.angular)