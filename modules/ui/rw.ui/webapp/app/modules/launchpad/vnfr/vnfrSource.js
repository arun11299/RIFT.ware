
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
let alt = require('../../core/alt');
import VnfrActions from './vnfrActions.js';
let Utils = require('../../utils/utils.js');
let API_SERVER = rw.getSearchParams(window.location).api_server;
let NODE_PORT = rw.getSearchParams(window.location).api_port || 3000;
let isSocketOff = true;
import $ from 'jquery';

export default {
  openVnfrSocket: function() {
    return {
      remote: function(state) {
        let nsrRegEx = new RegExp("([0-9a-zA-Z-]+)\/vnfr$");
        let nsr_id;
        try {
          console.log('NSR ID in url is', window.location.hash.match(nsrRegEx)[1]);
          nsr_id = window.location.hash.match(nsrRegEx)[1];
        } catch (e) {

        }
        return new Promise(function(resolve, reject) {
          if (state.socket) {
            resolve(false);
          }
          console.log(nsr_id)
          $.ajax({
            url: 'http://' + window.location.hostname + ':' + NODE_PORT + '/socket-polling?api_server=' + API_SERVER,
            type: 'POST',
            beforeSend: Utils.addAuthorizationStub,
            data: {
              url: API_SERVER + ':' + NODE_PORT + '/launchpad/nsr/' + nsr_id + '/vnfr?api_server=' + API_SERVER,
            },
            success: function(data) {
              var url = 'ws://' + window.location.hostname + ':' + data.port + data.socketPath;
              var ws = new WebSocket(url);
              resolve(ws);
            }
          });
        })
      },
      loading: VnfrActions.openVnfrSocketLoading,
      success: VnfrActions.openVnfrSocketSuccess,
      error: VnfrActions.openVnfrSocketError,
    }
  }
}
