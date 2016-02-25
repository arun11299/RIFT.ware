/**
 *
 * moduleLoader
 * ============
 *
 */

/*!*/
(function (window, console, angular, $script, undefined) {
  'use strict';
  var definitions;
  //This will be removed when we have a dedicated core module
  try {
    angular.module('core');
  } catch (e) {
    angular.module('core', []);
  }

  /** Creates the $rw global for storing the main app and navigation */
  if (typeof window.$rw === 'undefined') {
    window.$rw = {
      nav: [],
      definitions:definitions
    };
  }

  /** Storage for the module list names */
  var moduleList = [];
  /** Storage for module script src location */
  var moduleSrc = [];
  /** Storage for module CSS src location */
  var moduleCSS = [];
  /** Storage for ext file src location */
  var extFiles = [];
  /** Allows access to angular services outside of the application */
  var initInjector = angular.injector(['ng']);
  //For ajax calls
  var $http = initInjector.get('$http');
  //For promises
  var $q = initInjector.get('$q');

  /** Used only for unit tests */
  var $httpBackend;

  /** Encapsulated HTTP requests require this karma check to be placed in the code */
  //At some point should be refactored so that this is not necessary
  var path = './';
  if (typeof window.__karma__ !== 'undefined') {
    path += 'base/build/';
  }

  /**
   * Returns module object containing a list of modules, their source location, and the application nav
   * @param {Object} conf Applicaton configuration
   * @param {Object} def Module definition
   * @return {Array} list
   * @return {Array} Src
   * @return {Array} nav
   */
  function addModules(conf, def) {
    for (var i = 0; i < conf.config.length; i++) {
      var modules = def.applications[conf.config[i]];

      if (modules) {
        checkAndAdd(modules, def);
      } else {
        console.log('No modules loaded')
      }
    }
    return {
      list: moduleList,
      src: moduleSrc,
      css: moduleCSS,
      nav: window.$rw.nav,
      files: extFiles
    };
  }

  /**
   * Is run once module list has been generated and source added to .html
   * @param modData Module object returned from AddModules function
   * @param cb Callback for removing the curtain
   */
  function bootstrapAngular(modData, cb) {
    /**
     * Primary application. Loads core dependencies. Config driven modules added in 'modules'
     * Exposes app as global variable $rw.
     */
    angular.module('modules', modData.list);
    console.log(modData);
    window.$rw.main = angular.module('rift',
      // ['core', 'ui.router', 'oc.lazyLoad', 'modules', 'integration'])
      ['core', 'ui.router', 'oc.lazyLoad', 'modules'])
      .config(function ($stateProvider, $urlRouterProvider, $ocLazyLoadProvider) {
        window.$rw.urlRouterProvider = $urlRouterProvider;
        // Debug: true will enable console messaging concerning modules loaded through $ocLazyLoad
        $ocLazyLoadProvider.config({debug: false});
        // Defaults route to first nav item in nav list
        try{
          $urlRouterProvider
          .otherwise('/' +
          window.$rw.nav[0].name.toLowerCase() || 'splash');
        } catch(e) {
          console.log(e)
        }

      });
    /**
     * Time to rock and roll
     */
    angular.element(document).ready(function () {
      angular.bootstrap(angular.element('body'), ['rift']);
      if(cb){
        cb();
      }
    });
  }

  /**
   * Compare the applications list to the modules list in the definitions.json, lazy load js and css files,
   * and inject modules into the 'rift' module
   * @param object collection An object containing a list of required modules for an application
   * @param object def A reference to the object obtained by the definntions.json
   */
  function checkAndAdd(collection, def) {
    if (collection.hasOwnProperty('modules')) {
      var a = collection.modules;
      // debugger;
      // Lets get all the files needed for those modules you have listed.
      for (var i = 0; i < a.length; i++) {
        loadModule(a[i], def);
        }
      }

    }

  var loadModule = function(m, def) {
    // Have we loaded you yet?
    if (moduleList.indexOf(m) === -1) {
      var key = m;
      var mod = def.modules[key];
      moduleList.push(key);
      // Great, so we've added your name to our primary Angular module.
      // Do you have anything else we need to get loaded?
      if (key && mod) {
        moduleSrc.push(mod.src);
        // Do you have any other modules you require to function properly?
        if (mod.hasOwnProperty('depends') && mod.depends.length > 0) {
          for (var k = 0; k < mod.depends.length; k++) {
            loadModule(mod.depends[k], def);
          }

        }
        // Do you have any additional files you need loaded?
        if (mod.hasOwnProperty('files')) {
          var b = mod.files;
          for (var j = 0; j < b.length; j++) {
            // Is it a .js file?
            if ((/.js/).test(b[j])) {
              // Okay, have we already loaded you?
              if (moduleSrc.indexOf(b[j]) === -1) {
                // Great. Here's another file to load
                moduleSrc.push(b[j]);
              }
            } else {
              // Okay, you're not a .js file, but are you a CSS file?
              if (/.css/.test(b[j])) {
                // Okay, but have we loaded you before?
                if (moduleCSS.indexOf(b[j]) === -1) {
                  // Great. Here's another file to load
                  moduleCSS.push(b[j]);
                }
              } else {
                console.log(b[j])
                console.log('You\'ve tried to load a file type we do not yet recognize');
              }
            }
          }
        }
      }
    }
  }

    function isUniqueType(file, list) {
      return list.indexOf(file) === -1;
    }

    /**
     * Loads server generated config then loads specified modules. Returns a promise used for bootstrapping the application.
     * @param object definitions Reference to definitions.json
     * @param service $httpBackend A reference to the angularjs $httpBackend service. Used for testing.
     * @param function cb Callback for removing the curtain.
     * @returns {*}
     */
    function loadConfig(definitions, $httpBackend, cb) {
      return $q(function (resolve, reject) {
        if(typeof(rw) != 'undefined' && !!rw.getSearchParams(window.location.href).config){
        successFn({
"config":[rw.getSearchParams(window.location.href).config]
})
        } else {


        $http.get(path + 'launchpad/config')
          .success(successFn)
          .error(function () {
            $http.get(path + 'offline/config.json')
              .success(successFn);
          });
        }
        function successFn(res) {
          // Lets get all the important data we're going to need to load up this application.
          var modData = addModules(res, definitions);
          // If testing, bypass loading of additional scripts
          if ($httpBackend) {
            resolve(modData);
          } else {
            // Add the CSS dependencies, if there are any.
            if(modData.css.length > 0){
              for(var i = 0; i < modData.css.length; i ++) {
                loadCSS(modData.css[i]);
              }
            }
            // Add all modules and files to html page.
            $script(modData.src, 'modules');
            // If there was some data required
            if (modData.src.length !== 0) {
              // We will wait until everything is loaded up
              $script.ready('modules', function () {
                // Let's get this party started.
                bootstrapAngular(modData, cb);
              });
            } else {
              // No additional source files to add? No problem. Bootstrap time.
              console.log('no source', modData);
              bootstrapAngular(modData, cb);
            }
          }
        }
      });
    }

    /**
     * Invoked by init() and testInit()
     * @param httpBackend
     * @returns {*}
     */
    function loadDefinitions(httpBackend) {
      //Reference to ngMock service $httpBackend passed in only during testing
      $httpBackend = httpBackend;
      return $q(function (resolve, reject) {
        $http.get(path + 'offline/definitions.json').success(function (res) {
          definitions = res;
          resolve(res);
        });

      });
    }

    /**
     * For silo'd use.
     */
    function init(cb) {
      loadDefinitions(null).then(function (definitions) {
        loadConfig(definitions, null, cb);
      });
    }


    /**
     * $httpBackend passed in through Jasmine test
     * @param $httpBackend
     * @returns {*}
     */
    function testInit($httpBackend) {
      return $q(function (resolve, reject) {
        loadDefinitions($httpBackend).then(function (definitions) {
          loadConfig(definitions, $httpBackend, null).then(function (config) {
            resolve(config);
          });
        });
      });
    }

    /**
     *
     * @type {{testModules: testInit, loadModules: init}}
     */
    var returnObj = {
      testModules: testInit,
      loadModules: init,
      init: init
    };

    if (typeof module === 'object') {
      module.exports = returnObj;
    }

  function loadCSS(href){
    var ss = window.document.createElement('link'),
      head = window.document.getElementsByTagName('head')[0];

    ss.rel = 'stylesheet';
    ss.href = href;

    // temporarily, set media to something non-matching to ensure it'll
    // fetch without blocking render
    ss.media = 'only x';

    head.appendChild(ss);

    setTimeout( function(){
      // set media back to `all` so that the stylesheet applies once it loads
      ss.media = 'all';
    },0);
  }
  })
  (window, window.console, window.angular, window.$script);
