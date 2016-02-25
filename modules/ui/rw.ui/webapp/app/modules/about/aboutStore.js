
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var alt = require('../core/alt');
function aboutStore () {
  this.exportAsync(require('./aboutSource.js'));
  this.bindActions(require('./aboutActions.js'));
}

aboutStore.prototype.getAboutSuccess = function(list) {
  this.setState({
    aboutList:list
  })
  console.log('success', list)
};

aboutStore.prototype.getCreateTimeSuccess = function(time) {
	this.setState({
		createTime:time['rw-mc:create-time']
	})
	console.log('uptime success', time)
}

module.exports = alt.createStore(aboutStore);;

