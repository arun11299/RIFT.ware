/**
 * UE-Sim module
 * @fileoverview UE-Sim module parent
 *
 * This file currently serves only as an example.
 * The ue-sim loaded in the integration version of the webapp is from the integration folder.
 *
 */



;(function(window, angular) {
  "use strict";
  /**
   * Adding a new nav element with the name of "UE-Sim" with an icon.
   *
   */
  window.$rw.nav.push({
    "name": "UE-Sim",
    "icon": "icon-control-panel",
    "module": "uesimModule"
  });
  //TODO
  //scriptLog needs to be added to UI module
  //vnfService needs to be consolodiated.
  angular.module('uesimModule',['ui.router','scriptLog','vnfService','reactLog'])
    .config(function($stateProvider){
      $stateProvider.state('ue-sim',{
        url: '/ue-sim',
        controller:'tempUECtrl',
        controllerAs: 'ue',
        templateUrl:'modules/views/rw.uesim-page.tmpl.html'
      });
    })
    .controller('tempUECtrl', tempUECtrlFn)
    .directive('ueInfoBox', ueInfoBoxFn)
    .directive('ueScriptStats', ueScriptStatsFn)
    .filter('checkDefaultFilter', function () {
      return function (input) {
        if (typeof(input) === 'undefined') {
          return '…';
        }
        return input;
      }
    });

  ueScriptStatsFn.$inject = ['ueScriptFn'];
  function ueScriptStatsFn(ueScriptFn) {
    return {
      restrict: 'AE',
      controller: function () {

      }
    };
  }


  ueInfoBoxFn.$inject = ['ueService'];
  function ueInfoBoxFn(ueService) {
    return {
      restrict: 'EA',
      scope: {
        data: '=data'
      },
      controller: ueInfoBoxCtrl,
      controllerAs: 'ueinfo',
      bindToController:true
    };
  }

  //ueInfoBoxCtrl.$inject = ['ueService'];
  function ueInfoBoxCtrl() {

  }

  function ueInfoBoxLink(scope) {
    scope.toggleAuto = ueService.toggleAuto();
  }



  function tempUECtrlFn($scope, ueScript, ueService, $rootScope, scriptLog, vnfService) {
    var self = this,
      subscriptions = [];

    self.default = '…';
    self.script_running = 0;
    self.new = true;
    self.script_uptime = '0d:0h:0m:0s';
    self.imsiState = {
      "state": {}
    };
    self.scripts = [];
    self.currentScriptIndex = 0;
    self.currentScriptObject = {};
    self.saveScript = saveScript;
    self.dropCurtain = true;
    //this selectScript should be placed within a directive tied to the scriptedit box
    self.selectScript = selectScript;
    init();


    self.setTemplate = setTemplate;
    self.convertToHex = convertToHex;
    self.runScript = runScript;
    self.killScript = killScript;

    self.checkDefault = function (input) {
      if (typeof(input) === 'undefined') {
        return self.default;
      }
      return input;
    };

    //EVENT LISTENERS
    //Kills sockets on route change
    $rootScope.$on('$stateChangeStart',function () {
        ueScript.stopScriptStateSocket();
        ueScript.stopGetImsiState();
    });


    //Updates script service with currently selected script
    $scope.$watch('ue.currentScriptObject', function(n,o){
      console.log('current Script updated')
      ueScript.selectScript(n);
    });


    //JQUERY
    //SHOULD BE MOVED TO A DIRECTIVE
    /* DROP DOWN functionality
        this should be converted into a new directive
        or angular-ui dropdown component used.
     */
    $(document).click(function () {
      self.scriptModeToggle = false;
      self.scriptSelectedToggle = false;
      $scope.$apply();
    });
    $('#scriptSelected').click(function (e) {
      self.scriptSelectedToggle = !self.scriptSelectedToggle;
      self.scriptModeToggle = false;
      self.verbosityLevelToggle = false;
      e.stopPropagation();
      $scope.$apply();
    });
    $('#scriptMode').click(function (e) {
      self.scriptModeToggle = !self.scriptModeToggle;
      self.scriptSelectedToggle = false;
      self.verbosityLevelToggle = false;
      e.stopPropagation();
      $scope.$apply();
    });


    //FUNCTIONS

    function convertToHex (teid) {
      if (!teid) return;
      return '0x' + decimalToHexString(parseInt(teid));
      function decimalToHexString(number) {
        if (number < 0) {
          number = 0xFFFFFFFF + number + 1;
        }
        return number.toString(16).toUpperCase();
      }
    }


    function scriptStateFn(data) {
      if (data.output) {
        self.scriptState = data.output;

        if (self.script_running === 0 || self.script_running == 1) {

          if (self.script_running != 3) self.script_running = 2;
          //If server indicates that a script is currently running,
          // this grabs the data for that script and updates the UI to show what script is running
          ueScript.getScript(self.scriptState.script["script-id"]).done(function (data) {
            self.currentScriptObject = data["test-script"];
            ueScript.selectScript(self.currentScriptObject);
            console.log(self.scriptState)
            for (var i = 0; i < self.scripts.length; i++) {
              if (self.scripts[i].id === self.scriptState.script["script-id"]) {
                self.currentScriptIndex = i;
              }
            }
            $scope.$apply();
          });
        }
        if (data.output.script.state === "Terminated" || data.output.script.state === "Complete" || data.output.script.state === "Error") {
          $rootScope.$broadcast('ue.script.stopped');
          console.log('Script state is now: ' + data.output.script.state);

          if (data.output.script.state === "Error") console.log('ERROR: ', data);
          ueScript.stopScriptStateSocket();
          self.script_running = 0;
          if (self.imsiState) self.imsiState.state.mobility = "Disconnected";
          $scope.$apply();
        }
      }
    }

    function init() {
      //var x$ = jQuery;
      //populate environment data
      var envs = ueService.setEnvironmentVariables();

      envs.then(function () {
        //console.log('getting scrip list')
        //get script list
        ueScript.getList().done(function (data) {
          self.scripts = data.collection['rw-appmgr:test-script'];
          console.log(self.scripts)
          self.currentScriptObject = self.scripts[0];
          self.currentScriptIndex = 0;
          self.currentScriptID = self.currentScriptObject.id;
          $scope.$apply();
        });

        ueService.getTemplates().then(function (data) {
          self.templateList = data;
          self.selectedTemplate = data[0];
          //console.log(data)
          ueScript.setIMSI(data[0].imsi);
          imsiFn();
        });
        vnfService.get().then(function(data){
          self.vnfData = {
            ip: vnfService.ip,
            name: vnfService.name,
            type: vnfService.type
          };
          console.log(vnfService);
        });
      });
    }

  function saveScript(script){
      ueScript.saveScript(script);
  }

    function setTemplate(name) {
      //console.log(name)
      self.selectedTemplate = ueService.setTemplate(name).then(function (data) {
        var uesim = data["rw-appmgr:ue-sim"];
        self.selectedTemplate = uesim;

        ueScript.setIMSI(uesim.imsi);

      });
    }

    function imsiFn() {
      console.log('imsiFn running');
      if (!rw.api.offline) {
        ueScript.getImsiState(function (data) {
          console.log('imsiFn returned ', data)
          ueScript.imsiSubscription = true;
          if (data) {
            if (!data.hasOwnProperty('error')) {
              var imsiState = data["rw-appmgr:ue-state"];
              self.imsiState = imsiState;
              if (!self.script_running) {
                self.script_running = 1;
                var script_id = imsiState["group-id"];
                $rootScope.$broadcast('ue.script.running', {id: script_id});
                //ueScript.getScriptState(data["ue-state"]["group-id"], scriptStateFn);
                ueScript.getScriptStateSocket(script_id, scriptStateFn);
              }
              try {
                self.callID = scriptLog.getCallID();
              } catch(e){
                console.log(e)
              }
            } else {
              if (!self.new) {
                self.imsiState.state.mobility = "Disconnected";
              }
              //self.script_running = false;
            }
          } else {

          }
          $scope.$apply();

        });
      }else{
        if(ueScript.offlineRunning){
          offlineScriptStart ($scope,  self, $rootScope, ueScript);
        }
      }
    }

    function killScript () {
      if(self.script_running == 2) {
        self.script_running = 3;
        if(self.imsiState) self.imsiState.state.mobility = "Disconnected";
        if (!rw.api.offline) {
          ueScript.killScript();
        } else {
          $rootScope.$broadcast('ue.script.off');
          ueScript.offlineRunning = false;
          offlineScriptStop($scope, self);

        }
      }
    }
    function runScript () {
      self.script_running = 1;
      self.new = false;
      try {
        $('#scriptInfoLyt').scrollTo('#scriptCallLyt', {duration: '1000', offsetTop: '60'});
      } catch (e) {
        console.log(e);
      }
      if (!rw.api.offline) {
        ueScript.runScript(self.currentScriptObject, function (script_id) {
          ueScript.getScriptStateSocket(script_id, scriptStateFn);
        });
      } else {

        offlineScriptStart ($scope,  self, $rootScope, ueScript);
      }
    }
    function selectScript(s,i){
      self.currentScriptIndex = i;
      self.currentScriptObject = s;
    }
  }
require('./service/react_log_service.js');
require('./service/ue-sim-service.js');
require('./service/vnf_service.js');
require('./control/ue-sim-filter.js');
require('./directive/rw.ue-sim-react_log.dir.js');
require('./component/ue-sim-controller.js');
require('./component/ue-sim-directive.js');
})
(window, window.angular, $);

