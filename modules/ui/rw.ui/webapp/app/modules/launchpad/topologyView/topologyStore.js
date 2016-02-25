/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
import TopologyActions from './topologyActions.js';
import TopologySource from './topologySource.js';
// import source
var alt = require('../../core/alt');
var AuthActions = require('../../login/loginAuthActions.js');
class TopologyStore {
    constructor() {
        var self = this;
        // initial state
        this.isLoading = true;
        this.topologyData = {};
        this.socket = null;
        this.detailView = null;
        this.hasSelected = false;
        // bind action listeners
        this.bindListeners({
            handleLogout: AuthActions.notAuthenticated
        });
        this.bindActions(TopologyActions);

        // bind source listeners
        this.exportAsync(TopologySource);
        this.exportPublicMethods({
            selectNode: this.selectNode,
            closeSocket: this.closeSocket
        })
    }
    selectNode = (node) => {
        var apiType = {
            'nsr' : 'getRawNSR',
            'vdur' : 'getRawVDUR',
            'vnfr': 'getRawVNFR'
        }
        this.setState({
            hasSelected: true
        })
       this.getInstance()[apiType[node.type]](node.id, node.parent ? node.parent.id : undefined)
    }
    getRawSuccess = (data) => {
        this.setState({
            detailView: data
        });
    }
    getRawLoading = () => {

    }
    openNSRTopologySocketLoading() {}
    openNSRTopologySocketSuccess(connection) {
        let self = this;
        let connectionManager = (type, connection) => {
            if (!connection) {
                console.warn('There was an issue connecting to the ' + type + ' socket');
                return;
            }
            if (self.socket) {
                self.socket.close();
                self.socket = null;
            }
            self.setState({
                socket: connection
            });
            connection.onmessage = function(data) {
                self.setState({
                    topologyData: JSON.parse(data.data),
                    isLoading: false
                });
            };
        }

        connectionManager('nsr', connection);
    }
    openNSRTopologySocketError() {}
    handleLogout = () => {
        this.closeSocket();
    }
    closeSocket = () => {
        if (this.socket) {
            this.socket.close();
        }
        this.setState({
            socket: null
        })
    }
}
export default alt.createStore(TopologyStore);
