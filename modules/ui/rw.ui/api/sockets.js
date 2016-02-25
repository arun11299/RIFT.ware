
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
//SOCKET MANAGER
// test
//Supports localhost node polling subscriptions and pass through subscriptions to other websockets
//
//TODO REFACTOR: this needs to happen. there's too much boilerplate code in here.
//TODO Document after refactoring
//TODO Improved logging for debugging
//TODO List of URLS

var WebSocketServer = require('ws').Server,
  WebSocket = require('ws'),
  Request = require('request');
var _ = require('underscore');
var constants = require('./common/constants.js');
var Promise = require('promise');


var Subscriptions = function(){
  this.ID = 1;
  this.socketServers = {};
  this.freePorts = [];
  for (var i = 0; i < constants.SOCKET_POOL_LENGTH; i++) {
    this.freePorts[i] = constants.SOCKET_BASE_PORT + i;
  };
};



/**
 * [subscribe description]
 * @param  {Object}   req
 * @param  {String}   req.body.url May be http, https, or ws
 * @param  {Function} req.body.transform A function that will transform
 *                                      the data before sending it out
 *                                      through the socket. Receives one
 *                                      argument, which is the data
 *                                      returned from the subscription.
 * @param  {Function} callback Function that will receive the SubscriptionData reference object
 * @return {Object}   SubscriptionReference  An object containing the subscription information.
 * @return {Number} SubscriptionReference.id The subscription ID
 * @return {Number} SubscriptionReference.port The subscription port
 * @return {String} SubscriptionReference.socketPath The path the to socket.
 */
Subscriptions.prototype.subscribe = function(req, callback) {
  var self = this;
  var URL = req.body.url;
  var SubscriptionReference;
  var sessionId = req.session.id;

  return new Promise(function(resolve, reject) {

    if (!self.socketServers.hasOwnProperty(sessionId)) {
      self.createWebSocketServer().then(function(successData) {

        self.socketServers[sessionId] = successData;

        //Check for more generix regex
        switch(URL.match(/(.{2,5}):\/\//)[1]) {
          case 'http' : self.socketInstance(req, self.socketServers[sessionId].wss, PollingSocket);
            break;
          case 'https' : self.socketInstance(req, self.socketServers[sessionId].wss, PollingSocket);
            break;
          case 'ws' : self.socketPassThrough(req, self.socketServers[sessionId].wss, WebSocket);
            break;
        }

        return resolve({
          statusCode: 200,
          data: {
            id: self.socketServers[sessionId].id,
            port: self.socketServers[sessionId].port,
            socketPath:self.socketServers[sessionId].socketPath,
            sessionId: sessionId
          }
        });
      },
      function(errorData) {
        return reject({
          statusCode: 503,
          errorMessage: errorData.error
        });
      });
    } else {
      SubscriptionReference = {
        id: self.socketServers[sessionId].id,
        port: self.socketServers[sessionId].port,
        socketPath:self.socketServers[sessionId].socketPath,
        sessionId: sessionId
      };

      resolve({
        statusCode: 200,
        data: SubscriptionReference
      });
    }
  });
};

Subscriptions.prototype.createWebSocketServer = function() {
  var self = this;

  return new Promise(function(resolve, reject) {
    if (self.freePorts.length == 0) {
      // TODO: Send back a good error
      return reject({
        error: 'No more free ports available for a socket'
      });
    }
    var port = self.freePorts.shift();
    var socketPath = '/ws/' + self.ID;
    //Error handling for already used port?
    var wss = new WebSocketServer({
      path: socketPath,
      port: port
    });
    self.ID++;
    return resolve({
      id: self.ID,
      port: port,
      socketPath: socketPath,
      wss: wss
    });
  });
};

Subscriptions.prototype.socketInstance = function(req, wss, Type) {
  console.log('Creating a new socketInstance for:', req.body.url, 'sessionId:', req.session.id);
    var self = this;
    var Socket = null;
    var Connections = [];
    var Index = 0;
    var sessionId = req.session.id;
    var wssRef = wss;

    wss.on('connection', function(ws) {
      if (!Socket) {
        Socket = new Type(req, 1000, req.body);
      }
      ws.index = Index++;
      Connections.push(ws);

      ws.onclose = function() {
        Connections.splice(ws.index, 1);
        if (Connections.length == 0) {
          console.log('Closing socket, no more connections');
          try {
            Socket.close();
            self.freePorts.unshift(self.socketServers[sessionId].port);
            delete self.socketServers[sessionId];
            wssRef.close();
          } catch (e) {
            console.log('Error: ', e);
          }
          Index = 0;
          delete Socket;
        }
      };

      Socket.onmessage = function(data) {
        var i;
        var self = this;
        if (req.body.transform && req.body.transform.constructor.name == "String") {
          //someTransformObject[req.body.transform](data, send)
          //req.body.transform(data, send);
        } else {
          send(data);
        }

        function send(payload) {
          var is401 = JSON.parse(payload).statusCode == 401;
          for (i = Connections.length - 1; i >=0; i -= 1) {
              Connections[i].send(payload, function wsError(error) {
                if (error) {
                  console.log('Error sending: ', error);
                  // Clean up sockets
                  console.log('Closing Socket');
                  try {
                    Socket.close();
                  } catch (e) {
                    console.log('Error closing Socket')
                  }
                  Connections.splice(i, 1);
                }
              });
          };
          if(is401) {
              try {
                    Socket.close();
                } catch (e) {
                    console.log('Error closing Socket')
              }
          }
        }

      };
    });
};

function PollingSocket(req, interval, config) {
  console.log('a new PollingSocket has appeared for url', req.body.url, 'sessionId:', req.session.id);
  var self = this;
  self.isClosed = false;
  var requestHeaders = {};
  _.extend(requestHeaders,
      {
        'Authorization': req.get('Authorization')
      }
    );

  var pollServer = function() {
    Request({
      url: req.body.url,
      method: config.method || 'GET',
      headers: requestHeaders,
      json: config.payload,
    }, function(error, response, body) {
      if (error) {
        console.log('Error polling: ' + req.body.url);
      } else {
        if(!self.isClosed) {
          self.poll = setTimeout(pollServer, 1000 || interval);
          var data = response.body;
          if (self.onmessage) {
            self.onmessage(data);
          }
        }
      }
    });
  };
  pollServer();
};

PollingSocket.prototype.close = function() {
  console.log('Closing poll');
  var self = this;
  this.isClosed = true;
  clearTimeout(self.poll);
};


module.exports = Subscriptions;
