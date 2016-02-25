//requires rw.js library
(function(window, angular, rw, undefined) {
"use strict";
  angular.module('uesimModule')
    .factory('ueScript', ueScriptFn)
    .factory('ueService', ueServiceFn);

  ueScriptFn.$inject = ['$q','$timeout','ueService','$rootScope'];
  function ueScriptFn($q, $timeout, ueService, $rootScope) {
    var script_id = null,
      selected_script = null,
        colony_name = "trafgen",
        trafsim_service_name = "mme-enb",
        scriptStatePoll = true,
        scriptState,
        IMSI,
        imsiState;
    return {
      offlineRunning: false,
      getList: function(){
        return rw.api.get('/api/operational/ltesim/test-script', 'application/vnd.yang.collection+json');
      },
      getScript: function(name){
        return rw.api.get('/api/operational/ltesim/test-script/' + name);
      },

      getScriptID: function(callback){
        return script_id;
      },
      killScript: function(callback){
        var self = this;
        var envData = ueService.getEnvironmentData();
          return $q(function(resolve,reject) {
            rw.api.rpc('/api/operations/trafsim-stop',{
              "input": {
                "colony": {
                  "name": envData["colony_name"],
                  "trafsim-service": {
                    "name": envData["trafsim_service_name"],
                   "ue-sim": {
                     "imsi": IMSI
                   }
                  }
                }
              }
            }).done(function(data){
              //self.stopScriptStateSocket();
              //self.setScriptStatePoll(false)
              if(callback)callback();
              console.log('Script Killed')
            });
          });

      },
      saveScript: function(script){
        var payload = {
          "input":{
            "edit-script":{
              "script-id":script.id,
              "new-script-code":script.code
            }
          }
        }

        return $q(function(resolve,reject){
          rw.api.rpc('/api/operational/lte-sim', payload)
            .done(function(){
              resolve();
            });
        });
      },
      selectScript: function(script,callback){
       selected_script = script;
        if(callback)callback();
      },
      runScript: function(script, callback){
        scriptStatePoll = true;
        selected_script = script;
        var envData = ueService.getEnvironmentData();
        console.log('running ', selected_script)
        if(selected_script === null){
          return;
        }
        return $q(function(resolve,reject){
          rw.api.rpc('/api/operations/trafsim-start',{
            "input": {
              "colony": {
                "name": envData["colony_name"],
                "trafsim-service": {
                  "name": envData["trafsim_service_name"],
                  "execute-script": {
                    "id": selected_script.id,
                    "ue-sim-name": ueService.getSelectedTemplate().name
                  }
                }
              }
            }
          }).done(function(data){
            console.log(data)

            script_id = data.output.script["execution-id"];
            if(script_id){
              callback(script_id);
              $rootScope.$broadcast('ue.script.running',{id:script_id});
              resolve(script_id);

            }else{
              console.log('ERROR: No Script ID received in runscript command')
            }

          });

        });
      },
      getScriptStateSocket: function(id, callback){
        console.log('script state start')
        var envData = ueService.getEnvironmentData();
        var postData = {
          "input":{
            "colony":{
              "name": envData["colony_name"],
              "trafsim-service":{
                "name": envData["trafsim_service_name"],
                "execution-id": id
              }
            }
          }
        };
        try{
          scrervalptState.unsubscribe();
        }catch (e){

        }
        scriptState = new rw.api.SocketSubscriber('web/get');
        scriptState.subscribeMeta(callback, {
          url: '/api/operations/trafsim-show/',
          contentType: 'application/vnd.yang.operation+json',
          data: JSON.stringify(postData),
          accept: 'application/vnd.yang.operation+json',
          method: 'POST'
        })
      },
      imsiSubscription: false,
      getImsiState: function(callback){
        imsiState = new rw.api.SocketSubscriber('web/get');
        imsiState.subscribeMeta(callback, {
          url: '/api/operational/ltesim/ue-state/'+IMSI+'?deep'
        });

      },
      getScriptState: function(id,callback){
        //var socket = new rw.api.SocketSubscriber('web/get');
        var envData = ueService.getEnvironmentData();
        var postUrl = '/api/operations/trafsim-show/';
        //console.log(envData)
        var postData = {
          "input":{
            "colony":{
              "name": envData["colony_name"],
              "trafsim-service":{
                "name": envData["trafsim_service_name"],
                "execution-id": id
              }
            }
          }
        };
        pollScript(callback, {url: postUrl, data: postData})
      },
      setScriptStatePoll: function(state){
        scriptStatePoll = state;
      },
      stopScriptStateSocket: function (){
        console.log('killing socket');
        try{
          scriptState.unsubscribe();
        }catch (e){

        }

      },
      stopGetImsiState: function(){
        try{
          imsiState.unsubscribe();
        } catch(e){
          console.log('Non-fatal error: Trying to unsubscribe from IMSI state without having an open subscription')
        }

      },
      setIMSI: function(imsi){
        IMSI = imsi;
      }

    };

    function pollScript (callback, meta){

      pollScriptCall(callback, meta);
    }
    function pollScriptCall (callback, meta){
      $timeout(function(){
        fetchScriptState(meta).done(function(data){

          callback(data);
          if(!scriptStatePoll){
            console.log('script is done, exiting', scriptStatePoll)
            return;
          }else{
            pollScript(callback, meta);
          }

        })

      },1000)
    }
    function fetchScriptState(meta){
      return rw.api.rpc(meta.url,meta.data,function(){
        console.log('Fetch script ERORR');
        scriptStatePoll = false;
      });
    }
  }

  ueServiceFn.$inject = ['$q'];
  function ueServiceFn($q) {
    //var subscriptions = [];
    var selectedTemplate;
    var colony_name, trafsim_service_name;
    return {
      getEnvironmentData: function(){
        return {
          "colony_name":  colony_name,
          "trafsim_service_name": trafsim_service_name
        }
      },
      getSelectedTemplate:function(){
        return selectedTemplate;
      },
      getTemplates: function(){
            return $q(function (resolve, reject){
              rw.api.get('/api/operational/ltesim/ue-sim/','application/vnd.yang.collection+json').done(function(data){
                var tmp = data.collection["rw-appmgr:ue-sim"];
                selectedTemplate = tmp[0];
                resolve(tmp);
              });
            });
      },
      setEnvironmentVariables: function(){
        console.log('getting env variables')
        return $q(function (resolve, reject){
          rw.api.get('/api/running/colony/','application/vnd.yang.collection+json').done(function(data){
            var env = data.collection["rw-base:colony"][0];
            colony_name = env.name;
            trafsim_service_name = env["rw-appmgr:trafsim-service"][0].name;
            resolve(env);
          })
        })
      },
      setTemplate: function(name){
        return $q(function (resolve, reject){
          rw.api.get('/api/operational/ltesim/ue-sim/' + name,'application/vnd.yang.collection+json').done(function(data){
            selectedTemplate = data;
            resolve(data);
          });
        });
      }
    };

  }

})(window, window.angular, window.rw);

