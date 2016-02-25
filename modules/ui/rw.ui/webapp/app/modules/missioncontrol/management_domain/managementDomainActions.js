
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var alt = require('../../core/alt');
var managementDomainActions = alt.generateActions(
                                                'createSuccess',
                                                'createLoading',
                                                'createFail',
                                                'getPoolsSuccess',
                                                'getPoolsFail',
                                                'updateSuccess',
                                                'updateLoading',
                                                'updateFail',
                                                'getManagementDomainSuccess',
                                                'getManagementDomainFail',
                                                'deleteSuccess',
                                                'deleteFail',
                                                'validateError',
                                                'validateReset'
                                                );

module.exports = managementDomainActions;
