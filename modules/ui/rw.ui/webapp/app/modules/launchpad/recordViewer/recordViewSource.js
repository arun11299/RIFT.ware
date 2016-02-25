/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
import RecordViewActions from './recordViewActions.js';
let Utils = require('../../utils/utils.js');
import $ from 'jquery';
let API_SERVER = rw.getSearchParams(window.location).api_server;
let HOST = API_SERVER;
let NODE_PORT = rw.getSearchParams(window.location).api_port || 3000;
let DEV_MODE = rw.getSearchParams(window.location).dev_mode || false;

if (DEV_MODE) {
    HOST = window.location.protocol + '//' + window.location.hostname;
}

export default {
    getNSR() {
        return {
            remote: function(state, recordID) {

                return new Promise(function(resolve, reject) {
                    $.ajax({
                        url: 'http://' + window.location.hostname + ':' + NODE_PORT + '/launchpad/nsr/' + recordID + '?api_server=' + API_SERVER,
                        type: 'GET',
                        beforeSend: Utils.addAuthorizationStub,
                        success: function(data) {
                            resolve(data);
                        }
                    }).fail(function(xhr) {
                        //Authentication and the handling of fail states should be wrapped up into a connection class.
                        Utils.checkAuthentication(xhr.status);
                    });;
                });
            },
            loading: RecordViewActions.getNSRLoading,
            success: RecordViewActions.getNSRSuccess,
            error: RecordViewActions.getNSRError
        };
    },
    getRawVNFR() {
        return {
            remote: function(state, vnfrID) {
                return new Promise(function(resolve, reject) {
                    $.ajax({
                        url: 'http://' + window.location.hostname + ':' + NODE_PORT + '/api/operational/vnfr-catalog/vnfr/' + vnfrID + '?api_server=' + API_SERVER,
                        type: 'GET',
                        beforeSend: Utils.addAuthorizationStub,
                        success: function(data) {
                            resolve(data);
                        }
                    });
                })
            },
            loading: RecordViewActions.getRawLoading,
            success: RecordViewActions.getRawSuccess,
            error: RecordViewActions.getRawError
        }
    },
    getRawNSR() {
        return {
            remote: function(state, nsrID) {
                return new Promise(function(resolve, reject) {
                    $.ajax({
                        url: 'http://' + window.location.hostname + ':' + NODE_PORT + '/api/operational/ns-instance-opdata/nsr/' + nsrID + '?api_server=' + API_SERVER,
                        type: 'GET',
                        beforeSend: Utils.addAuthorizationStub,
                        success: function(data) {
                            resolve(data);
                        }
                    });
                })
            },
            loading: RecordViewActions.getRawLoading,
            success: RecordViewActions.getRawSuccess,
            error: RecordViewActions.getRawError
        }
    },
    getNSRSocket() {
        return {
            remote(state, recordID) {
                return new Promise(function(resolve, reject) {
                    console.log('opening nsr Socket')
                        // if (state.socket) {
                        //   resolve(false);
                        // }
                    $.ajax({
                        url: 'http://' + window.location.hostname + ':' + NODE_PORT + '/socket-polling?api_server=' + API_SERVER,
                        type: 'POST',
                        beforeSend: Utils.addAuthorizationStub,
                        data: {
                            url: HOST + ':' + NODE_PORT + '/launchpad/nsr/' + recordID + '?api_server=' + API_SERVER,
                        },
                        success: function(data) {
                            var url = 'ws://' + window.location.hostname + ':' + data.port + data.socketPath;
                            var ws = new WebSocket(url);
                            resolve(ws);
                        }
                    });
                })
            },
            loading: RecordViewActions.getNSRSocketLoading,
            success: RecordViewActions.getNSRSocketSuccess,
            error: RecordViewActions.getNSRSocketError
        }
    },
    getVNFRSocket() {
        return {
            remote(state) {
                return new Promise(function(resolve, reject) {
                    // if (state.socket) {
                    //   resolve(false);
                    // }
                    $.ajax({
                        url: 'http://' + window.location.hostname + ':' + NODE_PORT + '/socket-polling?api_server=' + API_SERVER,
                        type: 'POST',
                        beforeSend: Utils.addAuthorizationStub,
                        data: {
                            url: HOST + ':' + NODE_PORT + '/launchpad/vnfr/' + state.recordID + '?api_server=' + API_SERVER,
                        },
                        success: function(data) {
                            var url = 'ws://' + window.location.hostname + ':' + data.port + data.socketPath;
                            var ws = new WebSocket(url);
                            resolve(ws);
                        }
                    });
                })
            },
            loading: RecordViewActions.getVNFRSocketLoading,
            success: RecordViewActions.getVNFRSocketSuccess,
            error: RecordViewActions.getVNFRSocketError
        }
    },
    execNsConfigPrimitive() {
        return {
            remote(state, payload) {
                return new Promise(function(resolve, reject) {
                    $.ajax({
                        url: 'http://' + window.location.hostname + ':' + NODE_PORT + '/launchpad/exec-ns-config-primitive?api_server=' + API_SERVER,
                        type: 'POST',
                        data: payload,
                        beforeSend: Utils.addAuthorizationStub,
                        success: function(data) {
                            resolve(data);
                        }
                    });
                })
            },
            loading: RecordViewActions.execNsConfigPrimitiveLoading,
            success: RecordViewActions.execNsConfigPrimitiveSuccess,
            error: RecordViewActions.execNsConfigPrimitiveError
        }
    }
}