
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var alt = require('../../core/alt');
var createConfigAgentAccountActions = alt.generateActions(
                                                'createSuccess',
                                                'createLoading',
                                                'createFail',
                                                'updateSuccess',
                                                'updateLoading',
                                                'updateFail',
                                                'deleteSuccess',
                                                'deleteFail',
                                                'getConfigAgentAccountSuccess',
                                                'getConfigAgentAccountsSuccess',
                                                'getConfigAgentAccountsFail',
                                                'getConfigAgentAccountFail',
                                                'validateError',
                                                'validateReset'
                                                );

module.exports = createConfigAgentAccountActions;
