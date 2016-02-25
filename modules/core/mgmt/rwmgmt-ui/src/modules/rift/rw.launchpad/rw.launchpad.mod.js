/**
 * Launchpad module
 * @fileoverview Launchpad module
 * @class angular_module.MyModule
 * @memberOf angular_module
 * Currently the create page
 *
 */

require('./directives/rw.launchpad-create.dir.js');
require('./directives/rw.launchpad-create_build.dir.js');
require('./directives/rw.launchpad-create_choose.dir.js');
require('./directives/rw.launchpad-create_config.dir.js');
require('./directives/rw.launchpad-create_confirm.dir.js');
require('./directives/rw.launchpad-create_image.dir.js');
require('./directives/rw.launchpad-create_inputs.dir.js');
require('./directives/rw.launchpad-create_resources.dir.js');
require('./directives/rw.launchpad-create_step_location.dir.js');
require('./directives/rw.launchpad-show_environment_cards.dir.js');

require('./service/rw.launchpad-create.svc.js');
require('./service/rw.launchpad-show_environment_cards.svc.js');

require('./components/rw.launchpad-environment_card.comp.jsx')
(function (window, angular) {
  /**
   * Adding a new nav element with the name of "UE-Sim" with an icon.
   *
   */
    //window.$rw.nav.push({
    //  "name": "Create",
    //  "icon": "icon-control-panel",
    //  "module": "uesimModule"
    //});
  angular.module('launchPad', ['ui.router', 'dispatchesque', 'ngMockE2E', 'rwHelpers', 'ngWebsocket'])
    .config(function ($stateProvider) {
      $stateProvider.state('create', {
        url: '/create',
        template: '<div launchpad-create></div>'
      });
    })

    .run( function(localData, $websocket, getSearchParams) {
      var apiserver = '../../src/modules/rift/rw.launchpad/localdata/';
      var loc = new localData.init();
      var request = function (loc) {
        var req = new XMLHttpRequest();
        req.open('GET', apiserver + loc, false);
        req.send(null);
        return [req.status, req.response, {}];
      };

      // Registering launchpad api endpoints

      loc.add('GET', /[.+]?templates/, function () {
        var schema = {
          "status": "success",
          "templates": [
            "repeat(1,6)",
            {
              "id": "{{getRandomIntByDigitCount(15)}}",
              "name": "{{syllable(6)}}"
            }
          ],
          "code": 200
        };
        req = ['200', $rw.component.localdata.jsonParser(schema), {}];
        return req;
      });
      loc.add('GET', /[.+]?app-images/, function () {
        var schema = {
          "status": "success",
          "images": [
            "repeat(1,3)",
            {
              "id": "{{getRandomIntByDigitCount(15)}}",
              "name": "{{syllable(6)}}"
            }
          ],
          "code": 200
        };
        req = ['200', $rw.component.localdata.jsonParser(schema), {}];
        return req;
      });
      loc.add('GET', /[.+]?config/, function () {
        var req = new request('config.xml');
        return req;
      });
      loc.add('GET', /[.+]?pools/, function () {
        var schema = {
          "status": "success",
          "pools": [
            "repeat(1,3)",
            {
              "id": "{{getRandomIntByDigitCount(15)}}",
              "name": "{{syllable(6)}}"
            }
          ],
          "code": 200
        };
        req = ['200', $rw.component.localdata.jsonParser(schema), {}];
        return req;
      });
      loc.add('GET', /[.+]?environments/, function () {
        var req = new request('environments.json');
        return req;
      });

      loc.add('GET', /[.+]?template\/[0-9]+$/, function (type, URL) {
        var id = URL.match(/template\/([0-9]+)/)[1];
        var req = new request('templateData.json');
        req[1] = JSON.parse(req[1])[id.toString()];
        return req;
      });

      loc.add('GET', /[.+]?template\/[0-9]+\/fields/, function (type, URL) {
        var id = URL.match(/template\/([0-9]+)/)[1];
        var req = new request('fields.json');
        req[1] = JSON.parse(req[1])[id.toString()];
        return req;
      });

      loc.add('GET', /[.+]?template\/[0-9]+\/resources/, function (type, URL) {
        //var id = URL.match(/template\/([0-9]+)/)[1];
        //var req = new request('resourcesData.json');
        //console.log(req)
        //req[1] = JSON.parse(req[1])[id.toString()];
        var req;
        var schema = {
          "status": "success",
          "resources": [
            "repeat(4,6)",
            {
              "id": "{{getRandomIntByDigitCount(15)}}",
              "name": "{{syllable(6)}}",
              "ip_address":"{{ipaddress()}}",
              "comments": "{{syllable(10)}}",
              "controller": "{{syllable(10)}}",
              "os_image": "Existing",
              "reservation": "Available",
              "type": "generated.type"
            }
          ],
          "code": 200
        };
        req = ['200', $rw.component.localdata.jsonParser(schema), {}];
        return req;
      });
    });



})(window, window.angular);
