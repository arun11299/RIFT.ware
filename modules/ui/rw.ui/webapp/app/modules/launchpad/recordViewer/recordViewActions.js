
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
let alt = require('../../core/alt');

export default alt.generateActions(
                                   'getNSRSuccess','getNSRError','getNSRLoading',
                                   'getVNFRSocketLoading','getVNFRSocketError','getVNFRSocketSuccess',
                                   'getNSRSocketSuccess','getNSRSocketError','getNSRSocketLoading',
                                   'getRawSuccess','getRawError','getRawLoading',
                                   'loadRecord',
                                   'constructAndTriggerConfigPrimitive',
                                   'execNsConfigPrimitiveLoading', 'execNsConfigPrimitiveSuccess', 'execNsConfigPrimitiveError',
                                   );
