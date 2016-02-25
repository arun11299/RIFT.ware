/**
 * Compatibility Checker.
 * =========
 * Requires platformjs - included in vendor.js
 * @module compatibilityChecker
 */
(function( global, factory ) {

  if ( typeof module === "object" && typeof module.exports === "object" ) {
    // For CommonJS and CommonJS-like environments where a proper `window`
    // is present, execute the factory and get compatibilityChecker.
    // For environments that do not have a `window` with a `document`
    // (such as Node.js), expose a factory as module.exports.
    module.exports = factory( global, global.platform, true );
  } else {
    factory( global, global.platform );
  }

// Pass this if window is not defined yet
}(typeof window !== "undefined" ? window : this, function(window, p, noGlobal) {

  var compat = {};
  compat.check = check;
  /*
   Supported browsers

   Chrome 38+
   Firefox 34+
   Safari 8
   Internet Explorer 10, 11
   */
  var browsers = {
    'Chrome':38,
    'Firefox':34,
    'Safari':8,
    'IE': 10
  };

  /**
   * @description uses the platformjs to library to determine whether or not
   * @param cb Callback
   * @returns {boolean}
   */
  function check(cb, isTest){
    var isCompatible = parseFloat(p.version) >= browsers[p.name];
    if(cb && isCompatible){
      cb();
    } else{
      if(!isCompatible && !isTest){
        console.log('Your browser does not meet our requirements');
        window.location.pathname = 'upgrade.html';
      }
      return isCompatible;
    }
  }


  /**
   * Exposes compatibilityChecker
   */
  if ( typeof noGlobal === "undefined" ) {
    var $rw = window.$rw;
    if($rw) {
      $rw.compatibilityChecker = compat;
    } else {
      window.$rw = {
        compatibilityChecker: compat
      };
    }
  }

  return compat;
}));
