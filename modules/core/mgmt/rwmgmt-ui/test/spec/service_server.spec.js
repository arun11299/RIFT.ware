require('../../src/modules/rift/rw.iot/rw.iot.mod.js');


"use strict";
describe("IOT Server App",function() {
  var el, scope, appChannel;

  beforeEach(angular.mock.module('rw.iot'));
  beforeEach(inject(function($rootScope, $compile, radio) {
    appChannel = radio.channel('appChannel');
    scope = $rootScope.$new();
    el = '<iot-app-server></iot-app-server>';
    el = $compile(el)(scope);
    scope.$digest();
  }));


      it("Data updated", function() {
            appChannel.trigger('iot-update2');
            setTimeout(function(){
                  expect(el.eq(0).text()).toBe('2')
            }, 2000);
      });

});