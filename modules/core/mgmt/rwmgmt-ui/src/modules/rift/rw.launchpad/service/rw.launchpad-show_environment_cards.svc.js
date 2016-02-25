angular.module('launchPad')
  .factory('showEnvironmentCardsFactory', function ($http, dispatcher, getSearchParams, $websocket) {

    // todo: have separate handler for each
    var actionHandler = function (params) {
      console.log('Received event with params',
                params,
                '. To update view, emit data-updated on dispatcher');
    };

    var apiServer = getSearchParams.api_server || '';
    var apiUrl = apiServer + '/api/';

    var actionHandlers = {
      'console': actionHandler,
      'cli': actionHandler,
      'webui': actionHandler,
      'start': actionHandler,
      'stop': actionHandler,
      'restart': actionHandler,
      'delete': actionHandler
    };

    for (var key in actionHandlers) {
      dispatcher.watch(key, actionHandlers[key]);
    }

///////



    var variable = new socketHandler({
      url:'/ws/environments',
      events: {
        "message": function(data) {
          console.log(data)
        }
      }
    });


    function socketHandler(cfg) {

      var self = this;
      var ws;

      self.on = function() {
        if (ws) {
          ws.$close();
        }
        ws = null
        //window.WebSockets = MockSocket;
        ws = create();
        //ws.$open();
      };


      self.off = function() {
        if (ws) {
          ws.$close();
        }
        ws = null;
        //window.WebSockets = window._WebSockets;
        ws = create({
          openTimeout: 500,
          messageInterval: 1000,
          fixtures: {
            test: {
              data: 'test'
            }
          }

        });
        //ws.$open();
        setTimeout(function(){
          ws.$emit('message')
        }, 2000)
      };

      function create(offline) {
        var url;
        if (apiServer !== '') {
          url = apiServer.replace('http://', '');
        } else {
          url = window.location.host;
        }
        //console.log('ws://' + url + cfg.url, offline)
        return $websocket.$new({
          url: 'ws://' + url + cfg.url,
          protocols: [],
          mock: offline || false,
          overwrite:true
          })

        .$on('$message', function(data){
            console.log(data)
          })
          .$on('test', function(data){
            console.log(data)
          })
        //.$on('$open', function () {
        //  test.$emit('test');
        //})
      }

    }

    //For testing socket toggle;
    window.testSocket = variable;


    ////////






    // Return literal with functions exposed
    return {
      getEnvironments: function () {
        return $http.get(apiUrl + 'environments');
      },

      openEnvironmentsSocket: function (callback) {
        var apiServer = getSearchParams.api_server || '';
        var localdata = getSearchParams.localdata;
        var url;
        var ws;
        var intervalId;
        if (localdata === 'true') {
          localdata = true;
        } else {
          localdata = false;
        }

        if (apiServer !== '') {
          url = apiServer.replace('http://', '');
        } else {
          url = window.location.host;
        }


        //if (localdata) {
        //  ws = $websocket.$new({
        //    url: 'ws://' + url + '/ws/environments',
        //    mock: {
        //      fixtures: '/rwmgmt-ui/src/modules/rift/rw.launchpad/localdata/environments_socket.json'
        //    }
        //  });
        //
        //  // mock socket created, get some data in approx 1 sec
        //  intervalId = setTimeout(function () {
        //    ws.$emit('environments');
        //  }, 1000);
        //} else {
        //  ws = $websocket.$new({
        //    url: 'ws://' + url + '/ws/environments',
        //    protocols: []
        //  });
        //}

        //ws.$on('$open', function () {
        //  console.log('socket opened');
        //});
        //
        //
        //ws.$on('$message', function (payload) {
        //  if (localdata) {
        //    callback(payload.data);
        //  } else {
        //    callback(payload);
        //  }
        //});
        //ws.$on('environments', function() {
        //  console.log('socket updated')
        //})
        //ws.$on('$close', function (payload) {
        //  console.log('socket closed');
        //});
        //
        //ws.$on('$error', function (err) {
        //  console.log('socket error', err);
        //});
        //
        //
        //window.ws = ws;

        return ws;
      },

      closeEnvironmentsSocket: function (ws) {
        ws.$close();
      }
    }
  });