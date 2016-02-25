angular.module('launchPad')
  .factory('createStepService', function(createEnvironmentAPI, dispatchesque){
    var subscription = new dispatchesque();
    var step = {};
    var list = [];

    step.next = next;
    step.key = {
      'Choose': '<choose-template></choose-template>',
      'Build': '<build-template></build-template>',
      'Resources': '<resources-template></resources-template>',
      'Image': '<image-template></image-template>',
      'Config': '<config-template></config-template>',
      'Confirm': '<confirm-template></confirm-template>'
    };
    step.sub = sub;

    //Initial step - Should not be hard coded
    listMonitor('Choose');

    // Should template selection should determine next steps?

    return step;

    function next(key, cb){
      listMonitor(key);
      if (cb) {
        cb();
      };
      console.log('Current ENV data: ',createEnvironmentAPI.getEnvironment())
    }

    function listMonitor(item) {
      list.push(item);
      subscription.emit('list-update', item)
    }

    function sub(e, c){
      console.log('watching')
      return subscription.watch(e,c);
    }
  })
  .factory('createEnvironmentAPI', function(dispatchesque, $http, getSearchParams, localData){
    window.$test = {};
    window.$test.on = localData.on
    window.$test.off = localData.off;
    var subscription = new dispatchesque();
    var apiServer = getSearchParams.api_server || '';
    var apiUrl = apiServer + '/api/';
    var cachedResources;
    var configFile;
    var confirmData;
    var create = {};
    var currentTemplate;
    var environment = resetEnvironment();
    var leadVM;
    var selectedResources;

    // Subscriptions set
    var appImagesSub = sub('appImage-update', function(data) {
      environment.app_image = data.images[data.images.length - 1].id;
    });
    var resourcesSub = sub('resources-update', function(data) {

    });
    var templateFieldsSub = sub('template-fields-update', function(data) {
      environment.fields = data.fields;
    });
    var templateResourcesSub = sub('template-resources-update', function(data){
      //environment.resources = data;
      cachedResources = data;
    });
    var templateSub = sub('template-update', function(data){
      currentTemplate = data;
      environment.template_id = currentTemplate.template.id;
      environment.description = '';
    });

    // Exposing methods
    create.createEnvironment = createEnvironment;
    create.filterSelectedVM = filterSelectedVM;
    create.getConfig = getConfig;
    create.getConfirmData = getConfirmData;
    create.getEnvironment = getEnvironment;
    create.getPools = getPools;
    create.getResources = getResources;
    create.getTemplates = getTemplates;
    create.resourceCache = getCachedResources;
    create.setResources = setResources;
    create.setTemplate = setTemplate;
    create.sub = sub;
    create.updateConfig = updateConfig;
    create.updateEnvironment = updateEnvironment;

    return create;





    // Function definitions
    function createEnvironment(data,launch) {

      var newEnvironment = {};
      // Set data to send to confirmData
      newEnvironment.environment = data;
      newEnvironment.environment.status = launch ? "active" : "inactive";
      var resources = selectedResources;
      newEnvironment.resources = [];
      angular.forEach(resources, function(v,k) {
        if (!newEnvironment.lead) {
          if(!data.lead){
            newEnvironment.lead = v.name;
          } else {
            newEnvironment.lead = data.lead
          }
        }
        newEnvironment.resources.push(v.name);
      });
      delete newEnvironment.environment.lead;
      newEnvironment.environment.app_image = newEnvironment.environment.app_image.id;
      delete newEnvironment.app_image;
      $http.post(apiServer + '/api/environment', newEnvironment)
        .error(function(error) {
          console.log('ERROR!! Something horrible happened. Here\'s more info about it: ', error);
        })
        .then(function(data) {
          if(configFile){
            putConfig(data.data.environment.id, configFile);
          }
        });
    }

    function filterSelectedVM (item) {
      var isIn = selectedResources.indexOf(item.name) > -1;
      try {
        return isIn;
      }catch(e){
        return false;
      }
    }

    function getCachedResources(){
      return cachedResources;
    }

    function getConfig(error) {
      $http
        .get(apiServer + '/api/template/' + environment.template_id + '/config' +  reduceFieldsString(reduceFields(environment.fields)))
        .success(function(data){
          configFile = data;
          subscription.emit('config-update', data);
        })
        .error(function(){
          console.log('An error has occurred with the config data. Fields were present but the config generation failed.');
          if(error){
            error();
          }
        });
    }

    function getConfirmData(data) {
      confirmData = data;
      angular.forEach(selectedResources, function(v,k) {
        if (!leadVM) {
          leadVM = selectedResources[0];
        }
      })
      confirmData.lead = leadVM;
      confirmData.resources = cachedResources;
      return confirmData;
    }

    function getEnvironment() {
      return environment;
    }

    function getImages() {
      $http
        .get(apiServer + '/api/app-images')
        .success(function(data){
          subscription.emit('images-update', data.images);
        })
        .error(function(){
          console.log('An error has occurred');
        });
    }

    function getPools() {
      $http
        .get(apiServer + '/api/pools')
        .success(function(data){
          subscription.emit('pools-update', data.pools);
        })
        .error(function(){
          console.log('An error has occurred');
        });
    }

    function getResources(id, error) {
      var isList = (!id && (id !== 0));
      var url = apiUrl + ( isList ? 'resources' : 'template/' + id + '/resources');
      $http
        .get(url)
        .success(function(data){
          subscription.emit( (!isList ? 'template-' : '') + 'resources' + '-update', data.resources || data.acquired );
        })
        .error(function(){
          console.log('An error has occurred while getting the resources');
          if(error){
            error();
          }
        });
    }

    function getTemplates(id, action, error){
      var isList = (!id && (id !== 0));
      var url = apiUrl + ( isList ? 'templates' : 'template/' + id + (!!action ? '/' + action : ''));
      $http
        .get(url)
        .success(function(data){
          subscription.emit('template' + (isList?'s':'') + (!!action? '-' + action:'') + '-update', data);
        })
        .error(function(){
          console.log('An error has occurred while getting the templates');
          if(error){
            error();
          }
        });
    }

    function putConfig(id, configFile) {
      return $http.put(apiServer + '/api/environment/' + id + '/config', {config: configFile});
    }

    function reduceFields(fields) {
      var newFields = [];
      for (var i = 0; i < fields.length; i++) {
        if (fields[i].field_type != "group") {
          newFields.push(fields[i])
        } else {
          newFields = newFields.concat(reduceFields(fields[i].field_value))
        }
      }
      return newFields;

    }

    function reduceFieldsString(fields) {
      var s = '?';
      for (var i = 0; i < fields.length; i++) {
        s += fields[i].field_id + '=' + fields[i].field_value + '&'
      }
      return s;
    }

    function resetEnvironment() {
      return {
        "name": "",
        "description": "",
        "mode": "Production",
        "pool": "",
        "app_image": "",
        "config_file": "",
        "template_id": "",
        "vm_count": ""
      };
    }

    function setTemplate(id){
      getTemplates(id);
      getImages();
      getPools();
      getTemplates(id, 'fields', function(){
        environment.fields = false;
      });
      getResources(id);
    }

    function setResources(resources, lead) {
      environment.resources = resources;
      selectedResources = resources;
      leadVM = lead;
      //collapse selectedResources
      var newR = [];
      angular.forEach(selectedResources, function(v,k){
        newR.push(k);
      });
      console.log(newR)
      selectedResources = newR;
    }

    function sub(e,c){
      return subscription.watch(e,c);
    }

    function updateConfig(config) {
      configFile = config;
    }

    function updateEnvironment(env) {
      environment = env;
    }

    function saveEnvironment(launch){

    }

  });