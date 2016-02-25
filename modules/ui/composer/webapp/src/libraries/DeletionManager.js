
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
/**
 * Created by onvelocity on 10/2/15.
 */

import _ from 'lodash'
import d3 from 'd3'
import UID from './UniqueId'
import SelectionManager from './SelectionManager'
import CatalogItemsActions from '../actions/CatalogItemsActions'
import DescriptorModelFactory from './model/DescriptorModelFactory'

import '../styles/Animations.scss'

const DELETE = 26;
const BACKSPACE = 8;

function onlyUnique(value, index, self) {
	return self.indexOf(value) === index;
}

function createDeleteEvent(e, uid, eventName = 'cut') {
	const data = {
		bubbles: true,
		cancelable: true,
		detail: {
			uid: uid,
			originalEvent: e
		}
	};
	if (window.CustomEvent.prototype.initCustomEvent) {
		// support IE
		var evt = document.createEvent('CustomEvent');
		evt.initCustomEvent(eventName, data.bubbles, data.cancelable, data.detail);
		return evt;
	}
	return new CustomEvent(eventName, data);
}

export default class DeletionManager {

	static onDeleteKey(event) {
		const target = event.target;
		if (event.defaultPrevented) {
			return
		}
		if ((event.which === DELETE || event.which === BACKSPACE) && /^BODY|SVG|DIV$/i.test(target.tagName)) {
			event.preventDefault();
			DeletionManager.deleteSelected(event);
			return false;
		}
	}

	static deleteSelected(event) {

		// TODO refactor this to be a flux action e.g. move this code into ComposerAppActions.deleteSelected()

		const selected = SelectionManager.getSelections();

		// get a valid list of items to potentially remove via the cut event handler
		const removeElementList = selected.filter(d => d).filter(onlyUnique).reduce((r, uid) => {
			const elements = Array.from(document.querySelectorAll('[data-uid="' + uid + '"]'));
			return r.concat(elements);
		}, []).filter(d => d);

		if (removeElementList.length === 0 && selected.length > 0) {
			// something was selected but we did not find any dom elements with data-uid!
			console.error(`No valid DescriptorModel instance found on element. Did you forget to put data-uid={m.uid}`,
				selected.map(uid => Array.from(document.querySelectorAll(`[data-uid="${uid}"]`))));
		}

		// proactively update the UI for visual transitions
		d3.selectAll(removeElementList).classed('-with-animations deleteItemAnimation', true).transition().each('end', function () {
			d3.select(this);
		});

		SelectionManager.removeOutline();

		// give time for the delete animation to play out
		setTimeout(() => {

			// now actually update the model
			const invokedEventAlreadyMap = {};
			const failedToRemoveList = removeElementList.map(removedElement => {

				const uid = UID.from(removedElement);

				if (invokedEventAlreadyMap[uid]) {
					return
				}

				try {
					const cancelled = !removedElement.dispatchEvent(createDeleteEvent(event, uid));
					if (cancelled) {
						d3.select(removedElement).classed('-with-animations deleteItemAnimation', false).style({opacity: null});
						return removedElement;
					}

				} catch (error) {
					console.warn(`Exception caught dispatching 'cut' event: ${error}`,
						selected.map(uid => Array.from(document.querySelectorAll(`[data-uid="${uid}"]`))));
					return removedElement;
				}

			}).filter(d => d).filter(onlyUnique);

			SelectionManager.clearSelectionAndRemoveOutline();
			failedToRemoveList.forEach(d => SelectionManager.addSelection(d));
			SelectionManager.refreshOutline();

		}, 230);

	}

	static addEventListeners() {
		DeletionManager.removeEventListeners();
		document.body.addEventListener('keydown', DeletionManager.onDeleteKey);
	}

	static removeEventListeners() {
		document.body.removeEventListener('keydown', DeletionManager.onDeleteKey);
	}

}
