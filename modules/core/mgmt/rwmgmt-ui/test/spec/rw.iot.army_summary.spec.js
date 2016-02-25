require('../../src/modules/rift/rw.iot/rw.iot.mod.js');

describe("IOT Army Summary Test",function() {
  var el, scope;

  beforeEach(angular.mock.module('templates'));
  beforeEach(angular.mock.module('rw.iot'));

  beforeEach(inject(function($rootScope, $compile) {
    scope = $rootScope.$new();
    el =
      '<iot-army-summary></iot-army-summary>';
    el = $compile(el)(scope);
    scope.$digest();
  }));

  it('should contain the copy "hello army"', function() {
    var actual = el[0].innerHTML;
    expect(actual.indexOf('IOT Army Summary')).toBeGreaterThan(0);
  })
});

