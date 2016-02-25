
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var alt = require('../core/alt');
module.exports = alt.generateActions(
                                       'getNsrInstancesSuccess',
                                       'getNsrInstancesError',
                                       'getLaunchpadConfigSuccess',
                                       'getLaunchpadConfigError',
                                       'openNSRSocketLoading',
                                       'openNSRSocketSuccess',
                                       'openNSRSocketError',
                                       'setNSRStatusSuccess',
                                       'setNSRStatusError',
                                       'deleteNsrInstance',
                                       'deleteNsrInstanceSuccess',
                                       'deleteNsrInstanceError',
                                       'nsrControlSuccess',
                                       'nsrControlError',
                                       'slideNoStateChange',
                                       'slideNoStateChangeSuccess',
                                       'validateError',
                                       'validateReset',
                                       'deletingNSR'
                                       );
