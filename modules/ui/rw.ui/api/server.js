
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */

var cluster = require("cluster");
var cpu = require('os').cpus().length;
var clusteredLaunch = process.env.CLUSTER_SUPPORT || false;

if (cluster.isMaster && clusteredLaunch) {
    console.log(cpu, ' cpu\'s found');
    for (var i = 0; i < cpu; i ++) {
    	cluster.fork();
    }

    cluster.on('online', function(worker) {
        console.log("Worker Started pid : " + worker.process.pid);
    });
    cluster.on('exit', function(worker, code, signal) {
        console.log('worker ' + worker.process.pid + ' stopped');
    });
} else {
	var express = require('express');
	var app = express();
	var routes = require('./routes.js');
	var mcRoutes = require('./routes/mission-control.js');
	var launchpadRoutes = require('./routes/launchpad.js');
	var session = require('express-session');
	app.use(session({
		secret: 'ritio rocks',
		resave: true,
		saveUninitialized: true
	}));

	routes(app);
	mcRoutes(app);
	launchpadRoutes(app);

	app.listen(3000, function() {
	  console.log('started')
	});
}