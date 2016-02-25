
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var alt = require('../../core/alt');
var createCloudAccountActions = alt.generateActions(
                                                'createSuccess',
                                                'createLoading',
                                                'createFail',
                                                'updateSuccess',
                                                'updateLoading',
                                                'updateFail',
                                                'deleteSuccess',
                                                'deleteFail',
                                                'getCloudAccountSuccess',
                                                'getCloudAccountFail',
                                                'validateError',
                                                'validateReset'
                                                );

module.exports = createCloudAccountActions;
