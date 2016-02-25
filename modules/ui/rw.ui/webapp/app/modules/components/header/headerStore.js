/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */

import LoggingActions from '../../logging/loggingActions.js';
import LoggingSource from '../../logging/loggingSource.js';
import HeaderActions from './headerActions.js';
var alt = require('../../core/alt');

class HeaderStoreConstructor {
    constructor() {
        var self = this;
        this.validateErrorEvent = 0;
        this.validateErrorMsg = '';
        this.exportAsync(LoggingSource);
        this.bindActions(LoggingActions);
        this.bindActions(HeaderActions);
        this.exportPublicMethods({
            validateReset: self.validateReset
        })
    }
    getSysLogViewerURLError = () => {
        this.validateError("Log URL has not been configured.");
    }
    getSysLogViewerURLSuccess = (data) => {
        window.open(data.url);
    }
    validateError = (msg) => {
        this.setState({
            validateErrorEvent: true,
            validateErrorMsg: msg
        });
    }
    validateReset = () => {
        this.setState({
            validateErrorEvent: false
        });
    }
}

export default alt.createStore(HeaderStoreConstructor)
