
var React = require('react');
var exports = exportsFn;

if (typeof module == 'object') {
  module.exports = exports;
}
if (typeof window.$rw == 'object' && typeof window == 'object') {
  $rw.ui = exports;
} else {
  window.$rw = {
    ui: exports
  };
}
// UI Helper
function exportsFn(obj, element){

  var createRender = createRenderFn;
  var createElement = createElementFn;
  return React.render(createRender(obj), element);

  function createRenderFn(items) {
    var render;
    // For each item in the object
    for(var k in items){
      if(items.hasOwnProperty(k)){
        // Are you an array? If you are it means you're a container.
        if (Array.isArray(items[k])) {
          var nested = []
          // For each element you contain lets go through this process again
          for(var i = 0; i < items[k].length; i++){
            nested.push(createRender(items[k][i]));
          }
          // Now that we've built up a collection of your children,
          // lets give them to you as a prop
          // NOTE: All containers should have an 'elements' prop
          render = createElement($rw.component[k], {
            elements: nested
          })
        } else {
          // You're not a container? Okay, well then lets create an element.
          render = createElement($rw.component[k], items[k])
        }
      }
    }
    // Returning the constructed React element.
    return render;
  }

  function createElementFn(ref, obj){
    return React.createElement(ref, obj)
  }
}


