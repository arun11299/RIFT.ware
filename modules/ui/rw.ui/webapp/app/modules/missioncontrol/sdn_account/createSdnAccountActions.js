
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var alt = require('../../core/alt');
var createSdnAccountActions = alt.generateActions(
                                                'createSuccess',
                                                'createLoading',
                                                'createFail',
                                                'updateSuccess',
                                                'updateLoading',
                                                'updateFail',
                                                'deleteSuccess',
                                                'deleteFail',
                                                'getSdnAccountSuccess',
                                                'getSdnAccountFail',
                                                'validateError',
                                                'validateReset',
                                                'getSdnAccountsSuccess'
                                                );

module.exports = createSdnAccountActions;
