/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
import TopologyL2Actions from './topologyL2Actions.js';
import TopologyL2Source from './topologyL2Source.js';
var alt = require('../../core/alt');
class TopologyL2Store {
    constructor() {
        var self = this;
        this.topologyData = {
            nodes: [],
            links: [],
            network_ids: []
        };
        this.errorMessage = null;

        this.detailView = null;
        this.hasSelected = false;

        this.bindListeners({
            getTopologyApiSuccess: TopologyL2Actions.GET_TOPOLOGY_API_SUCCESS,
            getTopologyApiLoading: TopologyL2Actions.GET_TOPOLOGY_API_LOADING,
            getTopologyApiError: TopologyL2Actions.GET_TOPOLOGY_API_ERROR
        });
        // bind source listeners
        this.exportAsync(TopologyL2Source);
    }

    getTopologyApiSuccess = (data) => {
        this.setState({
            topologyData: data,
            errorMessage: null
        });
    }

    getTopologyApiLoading = () => {

    }

    getTopologyApiError = (errorMessage) => {
        this.errorMessage = errorMessage;
    }
}

export default alt.createStore(TopologyL2Store);
