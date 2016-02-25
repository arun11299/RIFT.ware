// Get a universally unique identifier
'use strict';

var count = 0;
module.exports = function uuid() {
  return 'react-tabs-' + count++;
};