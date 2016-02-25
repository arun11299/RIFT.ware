
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var alt = require('../core/alt');
var aboutActions = alt.generateActions(
                                                'getAboutSuccess',
                                                'getAboutLoading',
                                                'getAboutFail',
                                                'getCreateTimeSuccess',
                                                'getCreateTimeLoading',
                                                'getCreateTimeFail'
                                                );

module.exports = aboutActions;
