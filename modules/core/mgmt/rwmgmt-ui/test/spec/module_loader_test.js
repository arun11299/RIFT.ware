//var modLoad = require('moduleLoader');

var modLoad = require('../../src/base/services/module_loader.js');
describe('Module Loader', function(){
  "use strict";
  var $httpBackend;
  var modData;
  beforeEach(inject(function($injector) {

    $httpBackend = $injector.get('$httpBackend');
    jasmine.getJSONFixtures().fixturesPath='base/build/offline';
    $httpBackend.whenGET('offline/definitions.json').respond(
      getJSONFixture('definitions.json')
    );
    $httpBackend.whenGET('offline/config.json').respond(
      getJSONFixture('config.json')
    );
  }));

  beforeEach(function(done){
    modLoad.testModules($httpBackend).then(function(data) {
      modData = data;
      done();
    });
  });
  it("Compiles module list based on loaded definition and config files", function(){
      expect(modData).toBeDefined();
  });
});
