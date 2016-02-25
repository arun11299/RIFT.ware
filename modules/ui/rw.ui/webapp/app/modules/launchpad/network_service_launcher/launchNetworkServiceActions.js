
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var alt = require('../../core/alt');

export default alt.generateActions(
                                   'descriptorSelected',
                                   'nameUpdated',
                                   'omDatacenterUpdated',
                                   'getCatalogSuccess',
                                   'getCatalogError',
                                   'getVDUSuccess',
                                   'getVDUError',
                                   'getLaunchpadConfigSuccess',
                                   'getLaunchpadConfigError',
                                   'getCloudAccountSuccess',
                                   'getCloudAccountError',
                                   'getDataCentersSuccess',
                                   'getDataCentersError',
                                   'launchNSRLoading',
                                   'launchNSRSuccess',
                                   'launchNSRError'
                                   )
