/**
 * Application curtain control
 * =======
 * @namepsace applicationCurtain
 * @name applicationCurtain
 * @constructor applicationCurtain
 * @module applicationCurtain
 */
(function( global, factory ) {

  if ( typeof module === "object" && typeof module.exports === "object" ) {
    // For CommonJS and CommonJS-like environments where a proper `window`
    // is present, execute the factory and get compatibilityChecker.
    // For environments that do not have a `window` with a `document`
    // (such as Node.js), expose a factory as module.exports.
    module.exports = factory( global, true );
  } else {
    factory( global);
  }

// Pass this if window is not defined yet
}(typeof window !== "undefined" ? window : this, function(window, noGlobal){
"use strict";
  /**
   *
   * @type {{on: on, off: off, setup: setup}}
   */
  var appCurtain = {
    on: on,
    off: off,
    setup: setup
  };
  var document = window.document;
  var body;
  //Top level DIV for the curtain
  var curtain = document.createElement("DIV");
  curtain.id = "curtain";
  curtain.className = "on";

  //Assigns reference to body tag element when ready then inserts the curtain as the first element.
  if(document.readyState === "complete") {
    //If DOM is already loaded
    setup();
  } else {
    //Else create a listener for it to finish loading
    document.addEventListener("DOMContentLoaded", function() {
      setup();
    });
  }


  /**
   *
   */
  function off(){
    //console.log('curtain off');
    curtain.className = 'off';
  }

  /**  */
  function on(){
    //console.log('curtain on');
    curtain.className = 'on';
  }

  /**  */
  function setup(){
    body = document.querySelector('body');
    body.insertBefore(curtain, body.firstElementChild);
    on();
  }


  /**
   * Exposes applicationCurtain
   */
  if ( typeof noGlobal === "undefined" ) {
    var $rw = window.$rw;
    if($rw) {
      $rw.appCurtain = appCurtain;
    } else {
      window.$rw = {
        appCurtain: appCurtain
      };
    }
  }
  return appCurtain;

}));




