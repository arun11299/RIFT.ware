/*global SVGSVGElement, SVGPathElement*/
/**
 * Created by onvelocity on 2/6/16.
 */

'use strict';

import _ from 'lodash'
import d3 from 'd3'
import UID from './UniqueId'
import React from 'react'
import PathBuilder from './graph/PathBuilder'

const SELECTIONS = '::private::selections';

/**
 * SelectionManager provides two features:
 * 	1) given DescriptorModel instances mark them as 'selected'
 * 	2) given DOM elements draw an outline around them
 *
 * Note that the consumer must call addSelection(descriptor) and outlineElement(dom) seperately. This allows for complex
 * selections to be separate from the outline indicator. It also allows for selection sets that do not have a visual
 * outline associated with them.
 */
class SelectionManager {

	static get paused() {
		return this._paused;
	}

	static set paused(v) {
		this._paused = v;
	}

	static pause() {
		SelectionManager.paused = true;
		SelectionManager.removeEventListeners();
	}

	static resume() {
		SelectionManager.paused = false;
		SelectionManager.addEventListeners(SelectionManager.element);
	}

	static select(obj) {
		const isSelected = SelectionManager.isSelected(obj);
		if (!isSelected) {
			SelectionManager.clearSelectionAndRemoveOutline();
			SelectionManager.addSelection(obj);
			return true;
		}
	}

	static get reactPauseResumeMixin() {
		return {
			get preventDeselectOnClick () {
				return {
					onMouseDownCapture: () => {
						SelectionManager.pause();
					},
					// slight delay is to prevent a click from clearing the selection
					onMouseUp: () => {
						setTimeout(() => SelectionManager.resume(), 200);
					}
				};
			}
		};
	}

	static addEventListeners(element) {
		SelectionManager.removeEventListeners();
		this.element = element || this.element || document.body;
		this.element.addEventListener('mousedown', SelectionManager.onMouseDown);
		this.element.addEventListener('mousemove', SelectionManager.onMouseMove);
		this.element.addEventListener('click', SelectionManager.onClickCaptureUpdateSelection);
	}

	static removeEventListeners() {
		if (this.element) {
			this.element.removeEventListener('click', SelectionManager.onClickCaptureUpdateSelection);
			this.element.removeEventListener('mousedown', SelectionManager.onMouseDown);
			this.element.removeEventListener('mousemove', SelectionManager.onMouseMove);
		}
	}

	static onClickCaptureUpdateSelection(event) {
		if (event.defaultPrevented) {
			return
		}
		const target = SelectionManager.getClosestElementWithUID(event.target);
		if (SelectionManager.isSelected(target)) {
			return
		}
		if (SelectionManager.getSelections().length) {
			SelectionManager.clearSelectionAndRemoveOutline();
		}

		delete this._mouseDownPosition;

	}

	static onMouseDown(event) {
		this._mouseDownPosition = {
			clientX: event.clientX,
			clientY: event.clientY,
			timeStamp: event.timeStamp
		};
	}

	static onMouseMove(event) {
		if (!this._mouseDownPosition) {
			return;
		}
		const target = SelectionManager.getClosestElementWithUID(event.target);
		if (!SelectionManager.isSelected(target)) {
			return
		}
		if (this._mouseDownPosition.clientX - event.clientX ||
			this._mouseDownPosition.clientY - event.clientY) {
			SelectionManager.moveOutline();
		}
	}

	static isSelected(obj) {
		const uid = UID.from(obj);
		if (uid) {
			return SelectionManager[SELECTIONS].has(uid);
		}
	}

	static getSelections() {
		return Array.from(SelectionManager[SELECTIONS]).filter(d => d);
	}

	static addSelection(obj) {
		if (this.paused) {
			return;
		}
		const uid = UID.from(obj);
		if (uid) {
			SelectionManager[SELECTIONS].add(uid);
		}
	}

	static updateSelection(container, addOrRemove = true) {
		if (addOrRemove) {
			SelectionManager.addSelection(container);
		} else {
			SelectionManager.removeSelection(container);
		}
	}

	static removeSelection(obj) {
		if (this.paused) {
			return;
		}
		const uid = UID.from(obj);
		if (uid) {
			SelectionManager[SELECTIONS].delete(uid);
		}
	}

	static clearSelectionAndRemoveOutline() {
		if (this.paused) {
			return;
		}
		SelectionManager.removeOutline();
		const unselected = SelectionManager.getSelections();
		if (unselected.length) {
			SelectionManager[SELECTIONS].clear();
		}
		return unselected;
	}

	static removeOutline() {
		if (this.paused) {
			return;
		}
		Array.from(document.querySelectorAll('[data-outline-indicator-outline]')).forEach(n => d3.select(n).remove());
	}

	static refreshOutline() {
		setTimeout(() => {
			SelectionManager.resume();
			SelectionManager.removeOutline();
			SelectionManager.getSelections().forEach(uid => {
				Array.from(this.element.querySelectorAll(`[data-uid="${uid}"]`)).forEach(SelectionManager.outline.bind(this));
			});
		}, 100);
	}

	static moveOutline() {
		SelectionManager.getSelections().forEach(uid => {
			Array.from(this.element.querySelectorAll(`[data-uid="${uid}"]`)).forEach(SelectionManager.outline.bind(this));
		});
	}

	static outline(dom) {

		if (this.paused) {
			return;
		}

		const elements = Array.isArray(dom) ? dom : [dom];

		elements.map(SelectionManager.getClosestElementWithUID).filter(d => d).forEach(element => {

			if (element instanceof SVGElement) {
				SelectionManager.outlineSvg(element);
			} else {
				SelectionManager.outlineDom(element);
			}

		});

	}

	static outlineSvg(element) {

		const svg = element.viewportElement;

		const path = new PathBuilder();

		const dim = element.getBBox();
		const adjustPos = SelectionManager.sumTranslates(element);

		let w = dim.width + 11;
		let h = dim.height + 11;
		let top = adjustPos[1] - 8 + dim.y;
		let left = adjustPos[0] - 8 + dim.x;

		let square = path.M(5, 5).L(w, 5).L(w, h).L(5, h).L(5, 5).Z.toString();

		if (element instanceof SVGPathElement) {
			const t = d3.transform(d3.select(element).attr('transform')).translate;
			square = d3.select(element).attr('d');
			top = t[1];
			left = t[0];
		}

		const data = {
			top: top,
			left: left,
			path: square
		};

		const indicator = svg.querySelector(['[data-outline-indicator]']) || svg;

		const outline = d3.select(indicator).selectAll('[data-outline-indicator-outline]').data([data]);

		outline.enter().append('g').attr({
			'data-outline-indicator-outline': true,
		}).style({
			'pointer-events': 'none'
		}).append('path').attr({
			stroke: 'red',
			fill: 'transparent',
			'stroke-width': '1.5px',
			'stroke-linejoin': 'round',
			'stroke-dasharray': '4',
			d: d => d.path
		});

		outline.attr({
			transform: d => `translate(${d.left}, ${d.top})`
		});

		outline.exit().remove();

	}

	static outlineDom(element) {

		element = SelectionManager.getClosestElementWithUID(element);

		const offsetParent = SelectionManager.offsetParent(element);

		const dim = element.getBoundingClientRect();
		const w = dim.width + 11;
		const h = dim.height + 11;


		const path = new PathBuilder();
		const square = path.M(5, 5).L(w, 5).L(w, h).L(5, h).L(5, 5).Z.toString();

		let parentDim;

		parentDim = SelectionManager.offsetDimensions(offsetParent);

		const top = dim.top - parentDim.top;
		const left = dim.left - parentDim.left;
		const svg = d3.select(offsetParent).append('svg').attr({
			'data-outline-indicator-outline': true,
			width: dim.width + 20,
			height: dim.height + 23,
			style: `pointer-events: none; position: absolute; z-index: 999; top: ${top - 8}px; left: ${left - 8}px; background: transparent;`

		}).append('g').append('path').attr({

			stroke: 'red',
			fill: 'transparent',
			'stroke-width': '1.5px',
			'stroke-linejoin': 'round',
			'stroke-dasharray': '4',
			d: square // 'M 4 4 L 30 4 L 30 30 L 4 30 L 4 4 Z'

		});

	}

	static getClosestElementWithUID(element) {
		let target = element;
		while (target) {
			if (UID.from(target)) {
				return target;
			}
			if (target === target.parentNode) {
				return;
			}
			target = target.parentNode;
		}
	}

	static offsetParent(target) {
		while (target) {
			if ((d3.select(target).attr('data-offset-parent')) || target.nodeName === 'BODY') {
				return target;
			}
			target = target.parentNode;
		}
		return document.body;
	}

	/**
	 * Util for determining the widest child of an offsetParent that is scrolled.
	 *
	 * @param element
	 * @returns {{top: Number, right: Number, bottom: Number, left: Number, height: Number, width: Number}}
	 */
	static offsetDimensions (element) {

		const elementDim = element.getBoundingClientRect();
		const dim = {
			top: elementDim.top,
			right: elementDim.right,
			bottom: elementDim.bottom,
			left: elementDim.left,
			height: elementDim.height,
			width: elementDim.width
		};

		if (element.dataset && element.dataset.offsetWidth) {
			const widthSelector = element.dataset.offsetWidth;
			const overrideWidth = Array.from(element.querySelectorAll(widthSelector));
			dim.width = overrideWidth.reduce((width, child) => {
				const dim = child.getBoundingClientRect();
				return Math.max(width, dim.width);
			}, elementDim.width);
		}

		return dim;

	}

	static sumTranslates(element) {
		let parent = element;
		const result = [0, 0];
		while (parent) {
			const t = d3.transform(d3.select(parent).attr('transform')).translate;
			result[0] += t[0];
			result[1] += t[1];
			if (!parent.parentNode || /svg/i.test(parent.nodeName) || parent === parent.parentNode) {
				return result;
			}
			parent = parent.parentNode;
		}
		return result;
	}

}

// warn static variable to store all selections across instances
SelectionManager[SELECTIONS] = SelectionManager[SELECTIONS] || new Set();

export default SelectionManager;
