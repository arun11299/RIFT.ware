
var React = require('react');

var exports = {
  localdata: require('../services/jsonParser.js'),
  test: require('./test/test.js'),
  button: require('./button/rw.button.js'),
  textInput: require('./text-input/rw.text-input.js'),
  textArea: require('./text-area/rw.text-area.js'),
  checkBox: require('./check-box/rw.check-box.js'),
  radioButton: require('./radio-button/rw.radio-button.js'),
  React: React
};
if (typeof module == 'object') {
  module.exports = exports;
}
if (typeof window.$rw == 'object' && typeof window == 'object') {
  $rw.component = exports;
} else {
  window.$rw = {
    component: exports
  };
}
