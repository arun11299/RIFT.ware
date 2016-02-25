(function (window, angular) {

  "use strict";


  angular.module('rwHelpers')
    .filter('bits', function() {
      return function(n, optionalCapacity) {
        if (n === undefined || isNaN(n)) {
          return '';
        }
        var units = false;
        if (optionalCapacity === undefined) {
          optionalCapacity = n;
          units = true;
        }
        var suffixes = [
          ['Mbps' , 1000],
          ['Gbps' , 1000000],
          ['Tbps' , 1000000000],
          ['Pbps' , 1000000000000]
        ];
        for (var i = 0; i < suffixes.length; i++) {
          if (optionalCapacity < suffixes[i][1]) {
            return (numeral((n  * 1000) / suffixes[i][1]).format('0,0') + (units ? suffixes[i][0] : ''));
          }
        }
        return n + (units ? 'Bps' : '');
      };
    })
    .filter('fmt', function() {
      return function(n, fmtStr) {
        return numeral(n).format(fmtStr);
      };
    })
    .filter('ppsUtilizationMax', function() {
      return function(item) {
        var rate = item.rate / 10000;
        var max = item.max * 0.0015;
        return rate/max;
      };
    })
    .filter('mbpsAsPps', function() {
      return function(mbps) {
        var n = parseInt(mbps);
        return isNaN(n) ? 0 : rw.ui.fmt(n * 1500, '0a').toUpperCase();
      };
    })
    .filter('bpsAsPps', function() {
      return function(speed) {
        return parseInt(speed) * 0.0015;
      };
    })
    .filter('bytes', function() {
      return function(n, optionalCapacity) {
        if (n === undefined || isNaN(n)) {
          return '';
        }
        var units = false;
        if (optionalCapacity === undefined) {
          optionalCapacity = n;
          units = true;
        }
        var suffixes = [
          ['KB' , 1000],
          ['MB' , 1000000],
          ['GB' , 1000000000],
          ['TB' , 1000000000000],
          ['PB' , 1000000000000000]
        ];
        for (var i = 0; i < suffixes.length; i++) {
          if (optionalCapacity < suffixes[i][1]) {
            return (numeral((n * 1000) / suffixes[i][1]).format('0,0') + (units ? suffixes[i][0] : ''));
          }
        }
        return n + (units ? 'B' : '');
      };
    })
    .filter('status', function() {
      return function(s) {
        return s == 'OK' ? 'yes' : 'no';
      };
    })
    .filter('ppsUtilization', function() {
      return function(pps) {
        return pps ? numeral(pps / 1000000).format('0.0') : '';
      };
    })
    .filter('k', function() {
      return function(n) {
        return rw.ui.fmt(rw.ui.noNaN(n), '0a');
      };
    })
    .filter('fmtNum', function() {
      return function(s) {
        var n = parseInt(s);
        return isNaN(n) ? '' : numeral(n).format('0,00');
      };
    })
    .filter('nonNaN', function() {
      return function(n) {
        return isNaN(n) ? 0 : n;
      };
    })
    .filter('safePrefix', function() {
      return function(val, prefix) {
        if (val === undefined || val === null || val.length == 0) {
          return '';
        }
        return prefix + val;
      };
    })
    .filter('pctOrNotAvailable', function() {
      return function(val) {
        return val? val + '%' : 'N/A';
      }
    })
    .filter('valueOrNotAvailable', function() {
      return function(val) {
        return typeof(val) === 'undefined' ? 'N/A': val;
      }
    })
    // Example:
    //  { a : true, b : false, c : true }
    // returns
    //  'a c'
    // Useful in CSS class list
    //  class="{hide : !isOn, bold : isImportant} | tokenList"
    // NOTE: Brought over from polymer, angular might have different suggested way
    .filter('tokenList', function() {
      return function(obj) {
        var s = '';
        _.each(obj, function(cssClass, flag) {
          if (flag) {
            s += ' ' + cssClass;
          }
        });
        return s;
      };
    })
    .filter('none', function() {
      return function(v) {
        return v || 'none';
      };
    })

})(window, window.angular);
