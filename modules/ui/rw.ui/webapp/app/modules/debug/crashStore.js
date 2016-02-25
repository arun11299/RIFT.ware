
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var alt = require('../core/alt');
function crashStore () {
  this.exportAsync(require('./crashSource.js'));
  this.bindActions(require('./crashActions.js'));
}

crashStore.prototype.getCrashDetailsSuccess = function(list) {
  this.setState({
    crashList:list
  })
  console.log('success', list)
};

module.exports = alt.createStore(crashStore);;

