(function (window, angular) {

  "use strict";

  angular.module('rwHelpers', ['RecursionHelper','radio'])

    // TODO Move this to a library service
    .service('getSearchParams', function () {
      this.api_server = getSearchParams(window.location.search).api_server || '';
      this.tmpl_url = getSearchParams(window.location.search).tmpl_url || false;
      this.localdata = getSearchParams(window.location.search).localdata || false;
      function getSearchParams(url) {
        //var a = document.createElement('a');
        //a.href = url;
        var key_value;
        var params = {};
        var items = url.replace('?', '').split('&');
        for (var i = 0; i < items.length; i++) {
          if (items[i].length > 0) {
            key_value = items[i].split('=');
            params[key_value[0]] = key_value[1];
          }
        }
        return params;
      }
    })
    // .factory('RecursionHelper', ['$compile', function ($compile) {
    //   return {
    //     /**
    //      * Manually compiles the element, fixing the recursion loop.
    //      * @param element
    //      * @param [link] A post-link function, or an object with function(s) registered via pre and post properties.
    //      * @returns An object containing the linking functions.
    //      */
    //     compile: function (element, link) {
    //       // Normalize the link parameter
    //       if (angular.isFunction(link)) {
    //         link = {post: link};
    //       }

    //       // Break the recursion loop by removing the contents
    //       var contents = element.contents().remove();
    //       var compiledContents;
    //       return {
    //         pre: (link && link.pre) ? link.pre : null,
    //         /**
    //          * Compiles and re-adds the contents
    //          */
    //         post: function (scope, element) {
    //           // Compile the contents
    //           if (!compiledContents) {
    //             compiledContents = $compile(contents);
    //           }
    //           // Re-add the compiled contents to the element
    //           compiledContents(scope, function (clone) {
    //             element.append(clone);
    //           });

    //           // Call the post-linking function, if any
    //           if (link && link.post) {
    //             link.post.apply(null, arguments);
    //           }
    //         }
    //       };
    //     }
    //   };
    // }])
    .factory('localData', function($httpBackend, getSearchParams) {

      $httpBackend.whenGET(/\.html$/).passThrough();
      $httpBackend.whenGET(/\.json/).passThrough();
      $httpBackend.whenPOST(/.+/).passThrough();

      var localDataStore = [];
      var localData = {};

      localData.on = function () {
        console.log('Local data is on')
        for (var i = 0; i < localDataStore.length; i++) {
          localDataStore[i].on();
        }
      };

      localData.off = function () {
        console.log('Local data is off')
        for (var i = 0; i < localDataStore.length; i++) {
          localDataStore[i].off();
        }
      };

      localData.init = function() {

        var self = this;
        var stubs = [];
        var stub = stubConstructor;

        self.add = function (type, regex, res) {
          var self = this;
          stubs.push(new stub(type, regex, res))
        };

        self.on = function () {
          for (var i = 0; i < stubs.length; i++) {
            stubs[i].init();
          }
        };

        self.off = function (cb) {
          for (var i = 0; i < stubs.length; i++) {
            stubs[i].off();
          }
          if (cb) {
            cb();
          }
        };

        function stubConstructor(type, regex, res) {
          var self = this;
          var respond;
          var wsInterval, ws;
          if(type == 'WS') {

          } else {
            if (!respond) {
              respond = $httpBackend['when' + type.toUpperCase()](regex).respond(res);
            }
            self.init = function () {
              respond.respond(res);
            };
            self.on = function () {
              respond = respond.respond(res);
            };
            self.off = function () {
              respond = respond.passThrough();
            };
          }
          if (getSearchParams.localdata == 'true') {
            self.on();
          } else {
            self.off();
          }

        }
        localDataStore.push(self);

      };
      return localData;
    })
})(window, window.angular);

require('./js/dispatchesque.js');
require('./js/recursive.js');
require('./js/radio.js');
require('./filters/filters.js');

