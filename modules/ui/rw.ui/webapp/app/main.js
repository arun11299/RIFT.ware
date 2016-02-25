
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


  require.ensure(['./modules/missioncontrol/missioncontrol.js', './modules/launchpad/launchpad.js', './modules/login/login.js', './modules/core/app.js'], function() {
    var API_SERVER =  rw.getSearchParams(window.location).api_server;
    if (!API_SERVER) {
      window.location.href = "//" + window.location.host + '/index.html?api_server=http://localhost';
    }
    require('./modules/missioncontrol/missioncontrol.js');
    require('./modules/launchpad/launchpad.js');
    require('./modules/core/app.js');
    require('./modules/login/login.js');
    angular.bootstrap(document.getElementsByTagName('body'), ['app', 'login']);

  });
