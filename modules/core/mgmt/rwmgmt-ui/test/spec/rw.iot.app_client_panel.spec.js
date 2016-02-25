require('../../src/modules/rift/rw.iot/rw.iot.mod.js');
require('../../src/modules/rift/rw.helper/rw.helper.mod.js');

describe("IOT App Client Panel",function() {
  var el, scope, timeout
  beforeEach(angular.mock.module('rw.iot'));
  beforeEach(inject(function($rootScope, $compile, radio, $timeout) {
    scope = $rootScope.$new();
    timeout = $timeout;
    el =
      '<rw-iot-app-client-panel></rw-iot-app-client-panel>';
    el = $compile(el)(scope);
    scope.$digest();
    appChannel = radio.channel('rw.iot');
  }));

  it('should contain the value: 1', function() {
    appChannel.trigger('data:update', 1);
    timeout(function() {
      expect(el.eq(0).text()).toBe('1');
    });
  });

  it('should update the value to: 2', function() {
    appChannel.trigger('data:update', 2);
    timeout(function() {
      expect(el.eq(0).text()).toBe('2');
    });
  });
});

