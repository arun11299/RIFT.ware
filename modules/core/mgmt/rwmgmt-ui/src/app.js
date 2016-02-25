// require('integration');
var moduleLoader = require('moduleLoader');
var curtain = require('applicationCurtain');
var themeSwitcher = require('themeSwitcher');
console.log(themeSwitcher)
themeSwitcher.init();
require('compatibilityChecker').check( function(){
  moduleLoader.init(curtain.off);
});


//This needs to go somewhere else

var jsonParser = require('./base/services/jsonParser.js');

if (typeof module == 'object') {
  module.exports = jsonParser
}
if (typeof window.$rw == 'object' && typeof window == 'object') {
  $rw.l = jsonParser;
} else {
  window.$rw = {
    l: jsonParser
  };
}

