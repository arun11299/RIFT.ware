var component = require('../../src/base/components/components.js');
var ui = require('../../src/base/components/uiHelper.js');
var React = component.React;

"use strict";

var ReactTestUtils;
var components = ["button", "textInput", "textArea", "checkBox", "radioButton"];
//keyUp & blur removed
var events = ["click", "mouseDown", "mouseOver", "mouseOut", "mouseUp", "touchCancel", "touchEnd", "touchMove", "touchStart", "keyDown", "keyPress", "focus"];
//var components = ["checkBox", "textInput"];
//var events = ["click"];
describe("Btn Test",function() {
  beforeEach(function () {
    ReactTestUtils = React.addons.TestUtils;
  });

  afterEach(function (done) {
    React.unmountComponentAtNode(document.body);
    setTimeout(done);
  });
  for (var i = 0; i < components.length; i++) {
    for (var j = 0; j < events.length; j++) {

      (function (i, j) {
        var testHelper = function (event, component_name, ReactTestUtils) {
          var component_obj = {};
          var toOnEvent = function (str) {
            return "on" + str.charAt(0).toUpperCase() + str.slice(1);
          }
          var handlerString = toOnEvent(event);
          // Initializes the test component.
          if (component_name === "checkBox") {
            component_obj[handlerString] = function (e, obj) {
              obj.setState({isDisabled: true});
            };
            component_obj.checkboxes = [{
              label:"a"
            }]
            component_obj.label = "test";
          } else if (component_name === "radioButton") {
            component_obj[handlerString] = function (e, obj) {
              obj.setState({isDisabled: true});
            };
            component_obj.radiobuttons = [{
              label:"a"
            }]
            component_obj.label = "test";
          } else {
            component_obj[handlerString] = function (e, obj) {
              if (component_name === "textInput" && handlerString === "onBlur") {
              }
              obj.setState({isDisabled: true});
            };
          }


          var component = React.createElement($rw.component[component_name], component_obj);
          var componentRendered = ReactTestUtils.renderIntoDocument(component).getDOMNode();
          var displayTarget;
          var eventTarget;

          // Based on the component being tested, this sets what dom element that will receive the event
          // and what dom element's "data-state" will be set to "disabled"
          if (component_name == "textInput" || component_name == "textArea") {
            displayTarget = eventTarget = componentRendered.childNodes[0].childNodes[1];
          } else if (component_name == "checkBox"|| component_name === "radioButton") {
            displayTarget = componentRendered;
            eventTarget = componentRendered.childNodes[1].childNodes[0];

          } else {
            displayTarget = eventTarget = componentRendered;
          }
          ReactTestUtils.Simulate[event](eventTarget);
          expect(displayTarget.getAttribute("data-state")).toEqual("disabled");
        }
        var event = events[j]
        var component = components[i];
        it(components[i] + ": check " + event, function () {
          //console.log(event, component)
          testHelper(event, component, ReactTestUtils);
        });

        //it("Check onClick", function () {
        //    var btn = React.createElement($rw.component[components[i]], {
        //      onClick: function (e, obj) {
        //        obj.setState({isDisabled: true})
        //      }
        //    });
        //    var btnRendered = ReactTestUtils.renderIntoDocument(btn).getDOMNode();
        //    if (components[i] == "textInput" || components[i] == "textArea") {
        //      btnRendered = btnRendered.childNodes[0].childNodes[1];
        //    }
        //    if (components[i] == "checkBox") {
        //      btnRendered = btnRendered.childNodes[2].childNodes[0];
        //    }
        //    ReactTestUtils.Simulate.click(btnRendered);
        //    expect(btnRendered.getAttribute("data-state")).toEqual('disabled')
        //});


        //
        //it("Check onMouseUp", function () {
        //  for(var i = 0; i < components.length; i++) {
        //    var btn = React.createElement($rw.component[components[i]], {
        //      onMouseUp: function (e, obj) {
        //        obj.setState({isDisabled: true})
        //      }
        //    });
        //    var btnRendered = ReactTestUtils.renderIntoDocument(btn).getDOMNode();
        //    if (components[i] == "textInput" || components[i] == "textArea") {
        //      btnRendered = btnRendered.childNodes[0].childNodes[1];
        //    }
        //    ReactTestUtils.Simulate.mouseUp(btnRendered);
        //    expect(btnRendered.getAttribute("data-state")).toEqual('disabled')
        //  }
        //});
        //
        //it("Check onMouseDown", function () {
        //  for(var i = 0; i < components.length; i++) {
        //    var btn = React.createElement($rw.component[components[i]], {
        //      label: "button & icon",
        //      onMouseDown: function (e, obj) {
        //        obj.setState({isDisabled: true})
        //      }
        //    });
        //    var btnRendered = ReactTestUtils.renderIntoDocument(btn).getDOMNode();
        //    if (components[i] == "textInput" || components[i] == "textArea") {
        //      btnRendered = btnRendered.childNodes[0].childNodes[1];
        //    }
        //    ReactTestUtils.Simulate.mouseDown(btnRendered);
        //    expect(btnRendered.getAttribute("data-state")).toEqual('disabled')
        //  }
        //});
        //
        //it("Check onMouseOver", function () {
        //  for(var i = 0; i < components.length; i++) {
        //    var btn = React.createElement($rw.component[components[i]], {
        //      label: "button & icon",
        //      onMouseOver: function (e, obj) {
        //        obj.setState({isDisabled: true})
        //      }
        //    });
        //    var btnRendered = ReactTestUtils.renderIntoDocument(btn).getDOMNode();
        //    if (components[i] == "textInput" || components[i] == "textArea") {
        //      btnRendered = btnRendered.childNodes[0].childNodes[1];
        //    }
        //    ReactTestUtils.Simulate.mouseOver(btnRendered);
        //    expect(btnRendered.getAttribute("data-state")).toEqual('disabled')
        //  }
        //});
        //
        //it("Check onMouseOut", function () {
        //  for(var i = 0; i < components.length; i++) {
        //    var btn = React.createElement($rw.component[components[i]], {
        //      label: "button & icon",
        //      onMouseOut: function (e, obj) {
        //        obj.setState({isDisabled: true})
        //      }
        //    });
        //    var btnRendered = ReactTestUtils.renderIntoDocument(btn).getDOMNode();
        //    if (components[i] == "textInput" || components[i] == "textArea") {
        //      btnRendered = btnRendered.childNodes[0].childNodes[1];
        //    }
        //    ReactTestUtils.Simulate.mouseOut(btnRendered);
        //    expect(btnRendered.getAttribute("data-state")).toEqual('disabled')
        //  }
        //});
        //
        //it("Check onMouseDown", function () {
        //  for(var i = 0; i < components.length; i++) {
        //    var btn = React.createElement($rw.component[components[i]], {
        //      label: "button & icon",
        //      onMouseDown: function (e, obj) {
        //        obj.setState({isDisabled: true})
        //      }
        //    });
        //    var btnRendered = ReactTestUtils.renderIntoDocument(btn).getDOMNode();
        //    if (components[i] == "textInput" || components[i] == "textArea") {
        //      btnRendered = btnRendered.childNodes[0].childNodes[1];
        //    }
        //    ReactTestUtils.Simulate.mouseDown(btnRendered);
        //    expect(btnRendered.getAttribute("data-state")).toEqual('disabled')
        //  }
        //});
        //
        //it("Check onTouchCancel", function () {
        //  for(var i = 0; i < components.length; i++) {
        //    var btn = React.createElement($rw.component[components[i]], {
        //      label: "button & icon",
        //      onTouchCancel: function (e, obj) {
        //        obj.setState({isDisabled: true})
        //      }
        //    });
        //    var btnRendered = ReactTestUtils.renderIntoDocument(btn).getDOMNode();
        //    if (components[i] == "textInput" || components[i] == "textArea") {
        //      btnRendered = btnRendered.childNodes[0].childNodes[1];
        //    }
        //    ReactTestUtils.Simulate.touchCancel(btnRendered);
        //    expect(btnRendered.getAttribute("data-state")).toEqual('disabled')
        //  }
        //});
        //
        //it("Check onTouchEnd", function () {
        //  for(var i = 0; i < components.length; i++) {
        //    var btn = React.createElement($rw.component[components[i]], {
        //      label: "button & icon",
        //      onTouchEnd: function (e, obj) {
        //        obj.setState({isDisabled: true})
        //      }
        //    });
        //    var btnRendered = ReactTestUtils.renderIntoDocument(btn).getDOMNode();
        //    if (components[i] == "textInput" || components[i] == "textArea") {
        //      btnRendered = btnRendered.childNodes[0].childNodes[1];
        //    }
        //    ReactTestUtils.Simulate.touchEnd(btnRendered);
        //    expect(btnRendered.getAttribute("data-state")).toEqual('disabled')
        //  }
        //});
        //
        //it("Check onTouchMove", function () {
        //  for(var i = 0; i < components.length; i++) {
        //    var btn = React.createElement($rw.component[components[i]], {
        //      label: "button & icon",
        //      onTouchMove: function (e, obj) {
        //        obj.setState({isDisabled: true})
        //      }
        //    });
        //    var btnRendered = ReactTestUtils.renderIntoDocument(btn).getDOMNode();
        //    if (components[i] == "textInput" || components[i] == "textArea") {
        //      btnRendered = btnRendered.childNodes[0].childNodes[1];
        //    }
        //    ReactTestUtils.Simulate.touchMove(btnRendered);
        //    expect(btnRendered.getAttribute("data-state")).toEqual('disabled')
        //  }
        //});
        //
        //it("Check onTouchStart", function () {
        //  for(var i = 0; i < components.length; i++) {
        //    var btn = React.createElement($rw.component[components[i]], {
        //      label: "button & icon",
        //      onTouchStart: function (e, obj) {
        //        obj.setState({isDisabled: true})
        //      }
        //    });
        //    var btnRendered = ReactTestUtils.renderIntoDocument(btn).getDOMNode();
        //    if (components[i] == "textInput" || components[i] == "textArea") {
        //      btnRendered = btnRendered.childNodes[0].childNodes[1];
        //    }
        //    ReactTestUtils.Simulate.touchStart(btnRendered);
        //    expect(btnRendered.getAttribute("data-state")).toEqual('disabled')
        //  }
        //});
        //
        //it("Check onKeyDown", function () {
        //  for(var i = 0; i < components.length; i++) {
        //    var btn = React.createElement($rw.component[components[i]], {
        //      label: "button & icon",
        //      onKeyDown: function (e, obj) {
        //        obj.setState({isDisabled: true})
        //      }
        //    });
        //    var btnRendered = ReactTestUtils.renderIntoDocument(btn).getDOMNode();
        //    if (components[i] == "textInput" || components[i] == "textArea") {
        //      btnRendered = btnRendered.childNodes[0].childNodes[1];
        //    }
        //    ReactTestUtils.Simulate.keyDown(btnRendered);
        //    expect(btnRendered.getAttribute("data-state")).toEqual('disabled')
        //  }
        //});
        //
        //it("Check onKeyPress", function () {
        //  for(var i = 0; i < components.length; i++) {
        //    var btn = React.createElement($rw.component[components[i]], {
        //      label: "button & icon",
        //      onKeyPress: function (e, obj) {
        //        obj.setState({isDisabled: true})
        //      }
        //    });
        //    var btnRendered = ReactTestUtils.renderIntoDocument(btn).getDOMNode();
        //    if (components[i] == "textInput" || components[i] == "textArea") {
        //      btnRendered = btnRendered.childNodes[0].childNodes[1];
        //    }
        //    ReactTestUtils.Simulate.keyPress(btnRendered);
        //    expect(btnRendered.getAttribute("data-state")).toEqual('disabled')
        //  }
        //});
        //
        //it("Check onKeyUp", function () {
        //  for(var i = 0; i < components.length; i++) {
        //    var btn = React.createElement($rw.component[components[i]], {
        //      label: "button & icon",
        //      onKeyUp: function (e, obj) {
        //        obj.setState({isDisabled: true})
        //      }
        //    });
        //    var btnRendered = ReactTestUtils.renderIntoDocument(btn).getDOMNode();
        //    if (components[i] == "textInput" || components[i] == "textArea") {
        //      btnRendered = btnRendered.childNodes[0].childNodes[1];
        //    }
        //    ReactTestUtils.Simulate.keyUp(btnRendered);
        //    expect(btnRendered.getAttribute("data-state")).toEqual('disabled')
        //  }
        //});
        //
        //it("Check onFocus", function () {
        //  for(var i = 0; i < components.length; i++) {
        //    var btn = React.createElement($rw.component[components[i]], {
        //      label: "button & icon",
        //      onFocus: function (e, obj) {
        //        obj.setState({isDisabled: true})
        //      }
        //    });
        //    var btnRendered = ReactTestUtils.renderIntoDocument(btn).getDOMNode();
        //    if (components[i] == "textInput" || components[i] == "textArea") {
        //      btnRendered = btnRendered.childNodes[0].childNodes[1];
        //    }
        //    ReactTestUtils.Simulate.focus(btnRendered);
        //    expect(btnRendered.getAttribute("data-state")).toEqual('disabled')
        //  }
        //});
        //
        //it("Check onBlur", function () {
        //  for(var i = 0; i < components.length; i++) {
        //    //console.log(components[i]);
        //    var btn = React.createElement($rw.component[components[i]], {
        //      label: "button & icon",
        //      onBlur: function (e, obj) {
        //        console.log();
        //        obj.setState({isDisabled: true})
        //      }
        //    });
        //    var btnRendered = ReactTestUtils.renderIntoDocument(btn).getDOMNode();
        //    if (components[i] == "textInput" || components[i] == "textArea") {
        //      btnRendered = btnRendered.childNodes[0].childNodes[1];
        //    }
        //    ReactTestUtils.Simulate.blur(btnRendered);
        //    expect(btnRendered.getAttribute("data-state")).toEqual('disabled')
        //  }
        //});
      })(i, j);
    }
  }

  /*
   * Note. OnMouseEnter and OnMouseLeave cannot be simulated by the React util tools.  It is an open issue.
   * If and when fixed, write tests for them.
   */


});