(function(window, angular){
  "use strict";
  angular.module('uiModule')
    .directive('scriptBox', function($modal){
      return {
        restrict: 'AE',
        scope: {
          scripts: '=',
          scriptState: '=',
          currentScriptIndex:'=',
          currentScriptObject:'=',
          saveScript: '&'
        },
        bindToController:true,
        controllerAs: 'sb',
        controller:scriptBoxCtrl,
        templateUrl:'/modules/views/scriptbox.tmpl.html',
        replace:true
      };

      function scriptBoxCtrl (){
        var self = this;
        self.selectScript = selectScript;
        self.editScript = editScript;

        function editScript(edit_script){
          var modalInstance = $modal.open({
            templateUrl: '/modules/views/editScript.tmpl.html',
            controller: function($modalInstance, script){
              var script_edit = Object.create(script);
              this.script = script_edit;
              this.close = $modalInstance.close;
              this.save = function(){
                script.code = script_edit.code;
                self.saveScript({d:script});
                $modalInstance.close();
              };
            },
            resolve:{
              script: function(){
                return edit_script;
              }
            },
            controllerAs:'modal',
            size: 'lg',
            backdrop: 'static'
          });
        }

        function selectScript (index,script) {
          self.currentScriptObject = script;
          self.currentScriptIndex = index;
        }
      }

    })

    .directive('scriptPreview',[function(){
      return {
        restrict:'AC',
        scope: {
          preview: '='
        },
        template: '<code class="python" ng-bind="preview"></code>',
        link:function(s,e){
          s.$watch('preview',function(){
            hljs.highlightBlock(e[0].children[0]);
          });
        }
      };
    }]);
})(window, window.angular);
