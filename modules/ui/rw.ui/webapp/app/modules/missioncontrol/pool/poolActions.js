
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var alt = require('../../core/alt');
var poolActions = alt.generateActions(
                                                'getPoolsSuccess',
                                                'getPoolByCloudSuccess',
                                                'getPoolsFail',
                                                'createSuccess',
                                                'createLoading',
                                                'createFail',
                                                'editSuccess',
                                                'editLoading',
                                                'editFail',
                                                'deleteSuccess',
                                                'deleteFail',
                                                'getResourcesSuccess',
                                                'getResourcesFail',
                                                'validateError',
                                                'validateReset'
                                                );

module.exports = poolActions;
