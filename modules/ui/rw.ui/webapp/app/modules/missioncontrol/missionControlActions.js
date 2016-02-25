
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var alt = require('../core/alt');
var MissionControlActions = alt.generateActions(
                                                'getFederationsSuccess',
                                                'getFederationsFail',
                                                'getCloudAccountsSuccess',
                                                'getCloudAccountsFail',
                                                'getSdnAccountsSuccess',
                                                'getSdnAccountsFail',
                                                'startLaunchpadSuccess',
                                                'startLaunchpadFail',
                                                'stopLaunchpadSuccess',
                                                'stopLaunchpadFail',
                                                'openMGMTSocketSuccess',
                                                'openMGMTError',
                                                'validateError',
                                                'validateReset'
                                                );

module.exports = MissionControlActions;
