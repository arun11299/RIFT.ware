
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var alt = require('../core/alt');
var crashDetailsActions = alt.generateActions(
                                                'getCrashDetailsSuccess',
                                                'getCrashDetailsLoading',
                                                'getCrashDetailsFail',
                                                );

module.exports = crashDetailsActions;
