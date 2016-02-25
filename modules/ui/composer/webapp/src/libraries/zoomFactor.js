
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
'use strict';
export default function zoomFactor(element = document.body) {
	let factor;
	let rect = element.getBoundingClientRect();
	let physicalW = rect.right - rect.left;
	let logicalW = element.offsetWidth;
	factor = Math.round((physicalW / logicalW) * 100) / 100;
	return factor;
}
