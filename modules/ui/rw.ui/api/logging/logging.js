
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var request = require('../utils/utils.js').request;
var Promise = require('promise');
var utils = require('../utils/utils.js');
var _ = require('underscore');
var constants = require('../common/constants.js');

var foreverOn = true;

var Logging = {}

Logging.get = function(req) {
  var api_server = req.query['api_server'];
  return new Promise(function(resolve, reject) {
    request({
      uri: utils.confdPort(api_server) + '/api/config/logging/syslog-viewer',
      method: 'GET',
      headers: _.extend({},
        constants.HTTP_HEADERS.accept.data,
        {
          'Authorization': req.get('Authorization')
        }),
      forever: foreverOn
    },
    function(error, response, body) {
      if(error) {
        console.log('Logging.get failed. Error:', error);
        reject({
          statusCode: response ? response.statusCode : 404,
          errorMessage: 'Issue retrieving syslog-viewer url'
        });
      } else {
        var data;
        try {
          data = JSON.parse(response.body);
        } catch (e) {
          console.log('Logging.get failed while parsing response.body. Error:', e);
          reject({
            statusCode: 500,
            errorMessage: 'Error parsing response.body during Logging.get'
          });
        }
        resolve(data);
      }
    });
  });
};

module.exports = Logging;
