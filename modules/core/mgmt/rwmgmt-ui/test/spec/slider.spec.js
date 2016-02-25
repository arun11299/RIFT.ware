// Q: what files do i import
//var component = require('../../src/base/m/components.js');
//var ui = require('../../src/base/components/uiHelper.js');
require('../../lib/angular-ui-bootstrap-bower/ui-bootstrap.min.js')
require('../../lib/angular-ui-codemirror/ui-codemirror.min.js')
 var ui = require('../../src/modules/rift/rw.ui/rw.ui.mod.js');
var slider = require('../../src/modules/rift/rw.ui/component/slider.js');
var noui = require('../../lib/nouislider/jquery.nouislider.min.js')
"use strict";

// Q: What does this do
//var ReactTestUtils;
//var components = ["textInput", "button"];


describe("Slider Test",function() {
  var element, scope;

  beforeEach(angular.mock.module('uiModule'));

  beforeEach(inject(function($rootScope, $compile) {
    scope = $rootScope.$new();
    element =
      '<rw-slider></rw-slider>';
    element = $compile(element)(scope);
    scope.$digest();
  }));

    it("Check slide", function () {
      // Q: do i create a dom with a slider in it here
      // Q: this doesn't appear to get run but gulp but gulp
      // does wake-up when i edit this file
      expect(1).toEqual(1);

    });

});
