/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
import RecordViewActions from './recordViewActions.js';
import RecordViewSource from './recordViewSource.js';
// import source
import AppHeaderActions from '../../components/header/headerActions.js';
var alt = require('../../core/alt');
var AuthActions = require('../../login/loginAuthActions.js');
class RecordViewStore {
    constructor() {
        this.isLoading = true;
        this.cardLoading = true;
        this.detailLoading = true;
        //Reference to current socket
        this.socket = null;
        //Reference to current record type
        //"vnfr", "nsr", "default"
        this.recordType = "default";
        //Reference to current record ID
        //uuid or null
        this.recordID = null;
        //Record data
        //object or null
        this.recordData = null;
        this.nav = [];
        this.configPrimitives = [];
        this.bindListeners({
            handleLogout: AuthActions.notAuthenticated
        })
        this.bindActions(RecordViewActions);
        this.exportPublicMethods({
            constructAndTriggerVnfConfigPrimitive: this.constructAndTriggerVnfConfigPrimitive,
            constructAndTriggerNsConfigPrimitive: this.constructAndTriggerNsConfigPrimitive,
            validateInputs: this.validateInputs
        })
        this.exportAsync(RecordViewSource);
    }
    handleLogout = () => {

    }
    handleCloseSocket = () => {
        if (this.socket) {
            this.socket.close();
        }
    }
    loadRecord(record) {
        this.setState({
            cardLoading: true,
            recordID: record.id
        });
    }
    getNSRSocketLoading() {
        this.setState({
            cardLoading: true
        })
    }
    getVNFRSocketLoading() {
        this.setState({
            cardLoading: true
        })
    }
    getVNFRSocketSuccess(connection) {
        // debugger;
        connectionManager.call(this, 'vnfr', connection);
    }
    getNSRSocketSuccess(connection) {
        connectionManager.call(this, 'nsr', connection);
    }
    getRawLoading() {
        this.setState({
            detailLoading: true
        })
    }
    getRawSuccess(data) {
        this.setState({
            rawData: data,
            detailLoading: false
        })
    }
    getNSRSuccess(data) {
        let nav = [];
        let nsrs = data.nsrs[0];
        nav.push({
            name: nsrs.name,
            id: nsrs.id,
            nsd_name: nsrs.nsd_name,
            type: 'nsr'
        });
        nsrs.vnfrs.map(function(vnfr) {
            nav.push({
                name: vnfr["short-name"],
                id: vnfr.id,
                type: 'vnfr'
            })
        });
        this.setState({
            nav: nav,
            recordID: nsrs.id,
            isLoading: false,
        });
    }
    constructAndTriggerVnfConfigPrimitive(data) {
        this.validateInputs(data);
        let vnfrs = data.vnfrs;
        let vnfrIndex = data.vnfrIndex;
        let configPrimitiveIndex = data.configPrimitiveIndex;
        let payload = {};

        let configPrimitive = vnfrs[vnfrIndex]['vnf-configuration']['config-primitive'][configPrimitiveIndex];

        payload['name'] = configPrimitive['name'];
        payload['nsr_id_ref'] = this.state.recordID;
        payload['vnf-list'] = [];

        let parameters = [];
        configPrimitive['parameter'].map((parameter) => {
            parameters.push({
                name: parameter['name'],
                value: parameter['value']
            });
        });

        let vnfPrimitive = [];
        vnfPrimitive[0] = {
            name: payload['name'],
            parameter: parameters
        }

        payload['vnf-list'].push({
            'member-vnf-index': vnfrs[vnfrIndex]['member-vnf-index-ref'],
            'vnfr-id-ref': vnfrs[vnfrIndex]['id'],
            'vnf-primitive': vnfPrimitive
        })

        this.execNsConfigPrimitive(payload);
    }
    constructAndTriggerNsConfigPrimitive(data) {
        let nsConfigPrimitiveIndexToExecute = data.nsConfigPrimitiveIndex;
        let nsConfigPrimitives = data.nsConfigPrimitives;
        let nsConfigPrimitive = data.nsConfigPrimitives[nsConfigPrimitiveIndexToExecute];

        let payload = {
            name: nsConfigPrimitive['name'],
            nsr_id_ref: nsConfigPrimitive['nsr_id_ref'],
            'vnf-list': [],
            'parameter': [],
            'parameter-group': [],
        };

        let vnfList = [];
        nsConfigPrimitive['vnf-primitive-group'].map((vnf) => {

            let vnfPrimitiveList = []
            vnf['inputs'].map((vnfPrimitive) => {

                let parameterList = [];

                const filterAndAddByValue = (paramObj) => {
                    if (paramObj['value'] != undefined) {
                        parameterList.push({
                            name: paramObj.name,
                            value: paramObj.value
                        });
                    }
                    return paramObj['value'] != undefined;
                }

                vnfPrimitive['parameter'].filter(filterAndAddByValue);

                if (parameterList.length > 0) {
                    vnfPrimitiveList.push({
                        name: vnfPrimitive['name'],
                        index: vnfPrimitive['index'],
                        parameter: parameterList
                    });
                }
            });

            vnfList.push({
                'member_vnf_index_ref': vnf['member-vnf-index-ref'],
                'vnfr-id-ref': vnf['vnfr-id-ref'],
                'vnf-primitive': vnfPrimitiveList
            });
        });

        payload['vnf-list'] = vnfList;

        let nsConfigPrimitiveParameterGroupParameters = [];
        nsConfigPrimitive['parameter-group'] && nsConfigPrimitive['parameter-group'].map((nsConfigPrimitiveParameterGroup) => {
            let nsConfigPrimitiveParameters = [];
            nsConfigPrimitiveParameterGroup['parameter'] && nsConfigPrimitiveParameterGroup['parameter'].map((nsConfigPrimitiveParameterGroupParameter) => {
                if (nsConfigPrimitiveParameterGroupParameter['value'] != undefined) {
                    nsConfigPrimitiveParameters.push({
                        'name': nsConfigPrimitiveParameterGroupParameter.name,
                        'value': nsConfigPrimitiveParameterGroupParameter.value
                    });
                }
            });
            nsConfigPrimitiveParameterGroupParameters.push({
                'name': nsConfigPrimitiveParameterGroup.name,
                'parameter': nsConfigPrimitiveParameters
            });
        });

        payload['parameter-group'] = nsConfigPrimitiveParameterGroupParameters;

        let nsConfigPrimitiveParameters = [];
        nsConfigPrimitive['parameter'] ? nsConfigPrimitive['parameter'].map((nsConfigPrimitiveParameter) => {
            if (nsConfigPrimitiveParameter['value'] != undefined) {
                nsConfigPrimitiveParameters.push({
                    'name': nsConfigPrimitiveParameter.name,
                    'value': nsConfigPrimitiveParameter.value
                });
            }
        }):null;

        payload['parameter'] = nsConfigPrimitiveParameters;

        this.execNsConfigPrimitive(payload);
    }
    execNsConfigPrimitiveSuccess(data) {
    }
    validateInputs(data) {
        let nsConfigPrimitiveIndexToExecute = data.nsConfigPrimitiveIndex;
        let nsConfigPrimitives = data.nsConfigPrimitives;
        let nsConfigPrimitive = data.nsConfigPrimitives[nsConfigPrimitiveIndexToExecute];
        let isValid = true;
        //Check parameter groups for required fields
        nsConfigPrimitive['parameter-group'] && nsConfigPrimitive['parameter-group'].map((parameterGroup, parameterGroupIndex) => {
            let isMandatory = parameterGroup.mandatory != 'false';
            let optionalChecked = parameterGroup.optionalChecked;
            let isActiveOptional = (optionalChecked && !isMandatory);
            if (isActiveOptional || isMandatory) {
                parameterGroup['parameter'] && parameterGroup['parameter'].map((parameter, parameterIndex) => {
                    let msg = 'Parameter Group: ' + parameterGroup.name + ' is not valid';
                    validateParameter(parameter, msg);
                });
            }
        });

        //Check top level parameters for required fields
        nsConfigPrimitive['parameter'] && nsConfigPrimitive['parameter'].map((parameter, parameterIndex) => {
            // let isMandatory = parameter.mandatory != 'false';
            // if (isMandatory) {
                let msg = 'Parameter: ' + parameter.name + ' is not valid'
                validateParameter(parameter, msg)
            // }

        });
        function validateParameter(parameter, msg) {
            if (parameter.mandatory == "true") {
                    if (!parameter.value) {
                        console.log(msg);
                        if (!parameter['default-value']) {
                            AppHeaderActions.validateError(msg);
                            isValid = false;
                            return false;
                        } else {
                            parameter.value = parameter['default-value'];
                            return true;
                        }
                    }
            };
            return true;
        };
        return isValid;
    }

}


function connectionManager(type, connection) {
    let self = this;
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
            recordData: JSON.parse(data.data),
            recordType: type,
            cardLoading: false,
            // ,isLoading: false
        });
    };
}

export default alt.createStore(RecordViewStore);
