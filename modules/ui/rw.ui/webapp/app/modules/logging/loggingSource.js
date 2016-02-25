
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var API_SERVER = rw.getSearchParams(window.location).api_server;
var NODE_PORT = 3000;
var loggingActions = require('./loggingActions.js');
var Utils = require('../utils/utils.js');
import $ from 'jquery';

export default {
  getSysLogViewerURL: function() {
    return {
      remote: function(state, requester) {
        console.log('getting url')
        return new Promise(function(resolve, reject) {
          $.ajax({
            url: 'http://' + window.location.hostname + ':3000/logging/syslog-viewer?api_server=' + API_SERVER,
            type: 'GET',
            beforeSend: Utils.addAuthorizationStub,
            success: function(data) {
              if (typeof data["rw-log-mgmt:syslog-viewer"] == '') {
                reject({
                  requester: requester
                });
              } else {
                resolve({
                  url: data["rwlog-mgmt:syslog-viewer"],
                  requester: requester
                });
              }
            },
            error: function(e) {
              console.log('Error getting sylogViewerURL for requester', requester, ':', e);
              reject({
                requester: requester
              });
            }
          });
        });
      },
      success: loggingActions.getSysLogViewerURLSuccess,
      error: loggingActions.getSysLogViewerURLError
    }
  }
}
