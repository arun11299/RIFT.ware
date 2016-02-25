require('../../src/modules/rift/rw.iot/rw.iot.mod.js');

describe("IOT app Server Summary Test",function() {
  var el, scope;

  beforeEach(angular.mock.module('templates'));
  beforeEach(angular.mock.module('rw.iot'));

  beforeEach(inject(function($rootScope, $compile) {
    scope = $rootScope.$new();
    el =
      '<iot-army-details></iot-army-details>';
    el = $compile(el)(scope);
    scope.$digest();
  }));

  it('should contain the copy "hello app server"', function() {
    var actual = el[0].innerHTML;
    expect(actual.indexOf('IOT army Details')).toBeGreaterThan(0);
  })
});

