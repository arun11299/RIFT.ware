/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
import NetworkServiceActions from './launchNetworkServiceActions.js';
import NetworkServiceSource from './launchNetworkServiceSource.js';
import GUID from '../../../assets/js/guid.js';
var alt = require('../../core/alt');


class LaunchNetworkServiceStore {
    constructor() {
        this.nsd = [];
        this.vnfd = [];
        this.pnfd = [];
        this.name = "";
        this.sla_parameters = [];
        this.selectedNSDid;
        this.selectedCloudAccount = {};
        this.dataCenters = {};
        this.cloudAccounts = [];
        this.isLoading = false;
        this.isStandAlone = false;
        this.hasConfigureNSD = false;
        this['input-parameters'] = [];
        this.registerAsync(NetworkServiceSource);
        this.exportPublicMethods({
            getMockData: getMockData.bind(this),
            getMockSLA: getMockSLA.bind(this),
            saveNetworkServiceRecord: this.saveNetworkServiceRecord,
            updateSelectedCloudAccount: this.updateSelectedCloudAccount,
            updateSelectedDataCenter: this.updateSelectedDataCenter,
            updateInputParam: this.updateInputParam,
            resetView: this.resetView,
            nameUpdated: this.nameUpdated
        });
        this.bindActions(NetworkServiceActions);
        this.descriptorSelected = this.descriptorSelected.bind(this);
        this.nameUpdated = this.nameUpdated.bind(this);
    }
    nameUpdated = (name) => {
        this.setState({
            name: name
        })
    }
    updateSelectedCloudAccount = (cloudAccount) => {
        var newState = {
            selectedCloudAccount: cloudAccount
        };
        if (cloudAccount['account-type'] == 'openmano') {
            let datacenter = this.dataCenters[cloudAccount['name']][0];
            // newState.selectedDataCenter = datacenter;
            newState.dataCenterID = datacenter.uuid;

        }
        this.setState(newState);
    }
    updateSelectedDataCenter = (dataCenter) => {
        console.log('updateed', dataCenter)
        this.setState({
            dataCenterID: dataCenter
        });
    }
    resetView = () => {
        console.log('reseting state')
        this.setState({
            name: ''
        })
    }
    descriptorSelected(data) {
        let NSD = data[0];
        let VNFIDs = [];

        let newState = {
            selectedNSDid: NSD.id
        };
        //['input-parameter-xpath']
        if (NSD['input-parameter-xpath']) {
            newState.hasConfigureNSD = true;
            newState['input-parameters'] = NSD['input-parameter-xpath'];
        } else {
            newState.hasConfigureNSD = false;
            newState['input-parameters'] = null;
        }
        NSD["constituent-vnfd"].map((v) => {
            VNFIDs.push(v["vnfd-id-ref"]);
        })
        this.getInstance().getVDU(VNFIDs);
        this.setState(newState);
    }

    //Action Handlers
    getCatalogSuccess(catalogs) {
        let self = this;
        let nsd = [];
        let vnfd = [];
        let pnfd = [];
        catalogs.forEach(function(catalog) {
            switch (catalog.type) {
                case "nsd":
                    nsd.push(catalog);
                    try {
                        self.descriptorSelected(catalog.descriptors)
                    } catch (e) {}
                    break;
                case "vnfd":
                    vnfd.push(catalog);
                    break;
                case "pnfd":
                    pnfd.push(catalog);
                    break;
            }
        });
        this.setState({
            nsd, vnfd, pnfd
        });
    }
    getCloudAccountSuccess(cloudAccounts) {
        this.setState({
            cloudAccounts: cloudAccounts,
            selectedCloudAccount: cloudAccounts[0]
        })
    }
    getDataCentersSuccess(dataCenters) {
        let newState = {
            dataCenters: dataCenters
        };
        if (this.selectedCloudAccount['account-type'] == 'openmano') {
            newState.dataCenterID = dataCenters[this.selectedCloudAccount.name][0].uuid
        }
        this.setState(newState)
    }
    getLaunchpadConfigSuccess = (config) => {
        let isStandAlone = ((!config) || config["operational-mode"] == "STANDALONE");
        this.setState({
            isStandAlone: isStandAlone
        });
    }
    getVDUSuccess(VNFD) {
        this.setState({
            sla_parameters: VNFD
        })
    }
    saveNetworkServiceRecord(name, launch) {
        //input-parameter: [{uuid: < some_unique_name>, xpath: <same as you got from nsd>, value: <user_entered_value>}]
        /*
        'input-parameter-xpath':[{
                'xpath': 'someXpath'
            }],
         */
        let guuid = GUID();
        let payload = {
            id: guuid,
            "nsd-ref": this.state.selectedNSDid,
            "name": name,
            "short-name": "name",
            "description": "a description for " + guuid,
            "admin-status": launch ? "ENABLED" : "DISABLED"
        }
        if (this.state.isStandAlone) {
            payload["cloud-account"] = this.state.selectedCloudAccount.name;
        }
        if (this.state.selectedCloudAccount['account-type'] == "openmano") {
            payload['om-datacenter'] = this.state.dataCenterID;
        }
        if (this.state.hasConfigureNSD) {
            let ips = this.state['input-parameters'];
            let ipsToSend = ips.filter(function(ip) {
                if (ip.value && ip.value != "") {
                    ip.uuid = GUID();
                    delete ip.name;
                    return true;
                }
                return false;
            });
            if (ipsToSend.length > 0) {
                payload['input-parameter'] = ipsToSend;
            }
        }
        this.launchNSR({
            'nsr': [payload]
        });
    }
    launchNSRLoading() {
        this.setState({
            isLoading: true
        });
        console.log('is Loading', this)
    }
    launchNSRSuccess(data) {
        let tokenizedHash = window.location.hash.split('/');
        this.setState({
            isLoading: false
        });
        return window.location.hash = 'launchpad/' + tokenizedHash[2];
    }
    launchNSRError() {
        let tokenizedHash = window.location.hash.split('/');
        this.setState({
            isLoading: false
        });
        return window.location.hash = 'launchpad/' + tokenizedHash[2];
    }
    updateInputParam = (i, value) => {
        let ip = this['input-parameters'];
        ip[i].value = value;
        this.setState({
            'input-parameters': ip
        })
    }
}

function getMockSLA(id) {
    console.log('Getting mock SLA Data for id: ' + id);
    this.setState({
        sla_parameters: slaData
    });
}

function getMockData() {
    console.log('Getting mock Descriptor Data');
    this.setState({
        nsd: data.nsd,
        vnfd: data.vnfd,
        pnfd: data.pnfd
    });
}
export default alt.createStore(LaunchNetworkServiceStore);
