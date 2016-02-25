/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
/**
 * Draw edges by dragging a ConnectionPoint to VNFD, VLD or another ConnectionPoint.
 *
 * If VLD does not exist between two VNFDs then add one.
 *
 * CSS for this class is defined in DescriptorGraph.scss.
 *
 */

import d3 from 'd3'
import CatalogItemsActions from '../../actions/CatalogItemsActions'
import SelectionManager from '../SelectionManager'
import DescriptorModelFactory from '../model/DescriptorModelFactory'

const line = d3.svg.line()
	.x(d => {
		return d.x;
	})
	.y(d => {
		return d.y;
	});

function mouseWithinPosition(position, mouseCoordinates, scale = 1) {
	const x = mouseCoordinates[0] / scale;
	const y = mouseCoordinates[1] / scale;
	const withinXBoundary = position.left < x && position.right > x;
	const withinYBoundary = position.top < y && position.bottom > y;
	return withinXBoundary && withinYBoundary;
}

function getContainerUnderMouse(comp, element, scale) {
	// determine if there is a connection point
	const dstConnectionPoint = comp.connectionPoints().filter(d => {
		if (d.type === 'connection-point') {
			return mouseWithinPosition(d.position, d3.mouse(element), scale);
		}
	});
	if (dstConnectionPoint[0].length) {
		return dstConnectionPoint;
	} else {
		// otherwise determine if destination is a VLD
		const dstVirtualLink = comp.descriptors().filter(d => {
			return d.type === 'vld' && mouseWithinPosition(d.position, d3.mouse(element), scale);
		});
		if (dstVirtualLink[0].length) {
			return dstVirtualLink;
		}
	}
}

export default class DescriptorGraphPathBuilder {

	constructor(graph) {
		this.graph = graph;
		this.containers = [];
	}

	descriptors() {
		return this.graph.containersGroup.selectAll('.descriptor');
	}

	connectionPoints() {
		return this.graph.connectorsGroup.selectAll('.connector');
	}

	paths() {
		return this.graph.g.selectAll('.connection');
	}

	addContainers(containers) {
		this.containers = this.containers.concat(containers);
	}

	static addConnection(srcSelection, dstSelection) {

		// return true on success; falsy otherwise to allow the caller to clean up accordingly

		if (srcSelection[0].length === 0 || dstSelection[0].length === 0) {
			return false;
		}

		const srcConnector = srcSelection.datum();
		const dstConnector = dstSelection.datum();

		// dstConnector can be a Connection Point or a VLD - branch accordingly

		if (srcConnector.parent === dstConnector.parent) {
			// connection points cannot connect to same VNFD
			return false;
		}

		const root = srcConnector.getRoot();

		if (root) {
			const vld = root.createVld();
			root.removeAnyConnectionsForConnector(srcConnector);
			root.removeAnyConnectionsForConnector(dstConnector);
			vld.addConnectionPoint(srcConnector);
			vld.addConnectionPoint(dstConnector);

			// notify catalog items have changed to force a redraw and update state accordingly
			CatalogItemsActions.catalogItemDescriptorChanged(root);

			return true;
		}

	}

	static addConnectionToVLD(srcSelection, dstSelection) {

		if (srcSelection[0].length === 0 || dstSelection[0].length === 0) {
			return false;
		}

		const dstVirtualLink = dstSelection.datum();
		const srcConnectionPoint = srcSelection.datum();

		dstVirtualLink.addConnectionPoint(srcConnectionPoint);

		// notify catalog items have changed to force a redraw and update state accordingly
		CatalogItemsActions.catalogItemDescriptorChanged(dstVirtualLink.getRoot());

		return true;

	}

	render() {

		/*
			Strategy: compare mouse position with the position of all the selectable elements.
			On mouse down:
			    determine if there is a connection point under the mouse (the source)
			    determine if there is already a path connected on this source
			On mouse move:
			    draw a tracer line from the source to the mouse
			On mouse up:
			    determine if there is a connection point or VLD under the mouse (the destination)
			    take the respective action and notify
		 */

		const comp = this;
		const drawLine = line.interpolate('basis');

		comp.boundMouseDownHandler = function (d) {

			let hasInitializedMouseMoveHandler = false;

			//SelectionManager.clearSelectionAndRemoveOutline();

			const srcConnectionPoint = comp.connectionPoints().filter(d => {
				if (/connection-point/.test(d.type)) {
					return mouseWithinPosition(d.position, d3.mouse(this), comp.graph.scale);
				}
			});

			if (srcConnectionPoint[0].length) {

				const src = srcConnectionPoint.datum() || {};

				// determine if there is already a path on this connection point
				// if there is then hide it; the mouseup handler will clean up
				const existingPath = comp.paths().filter(d => {
					return d && d.parent && d.parent.type === 'vld' && d.key === src.key;
				});

				// create a new path to follow the mouse
				const path = comp.graph.paths.selectAll('.new-connection').data([srcConnectionPoint.datum()], DescriptorModelFactory.containerIdentity);
				path.enter().append('path').classed('new-connection', true);
				comp.boundMouseMoveHandler = function () {

					const mouseCoordinates = d3.mouse(this);

					path.attr({
						fill: 'red',
						stroke: 'red',
						'stroke-width': '4px',
						d: d => {
							const srcPosition = d.position.centerPoint();
							const dstPosition = {
								x: mouseCoordinates[0] / comp.graph.scale,
								y: mouseCoordinates[1] / comp.graph.scale
							};
							return drawLine([srcPosition, dstPosition]);
						}
					});

					if (!hasInitializedMouseMoveHandler) {
						hasInitializedMouseMoveHandler = true;

						existingPath.style({
							opacity: 0
						});

						// allow for visual treatment of this drag operation
						srcConnectionPoint.classed('-selected', true);

						// allow for visual treatment of this drag operation
						comp.graph.svg.classed('-is-dragging-connection-point', true);

						// identify which descriptors are a valid drop target
						comp.descriptors().filter(d => {
							if (srcConnectionPoint.datum().key === d.key) {
								return false;
							}
							if (d.type === 'vld') {
								return true;
							}
							return false;
						}).classed('-is-valid-drop-target', true);

						// identify which connection points are a valid drop target
						comp.connectionPoints().filter(d => {

							const src = srcConnectionPoint.datum();
							const root = src.getRoot();
							const isNSD = DescriptorModelFactory.isNetworkService(root);
							if (isNSD) {
								return !(src.key === d.key || src.parent.key === d.parent.key || d.type === 'internal-connection-point');
							}

							return true;

							const notOverSelf = src.key !== d.key;
							const isInternalCP = DescriptorModelFactory.isInternalConnectionPoint(d);
							const isInternalVLD = DescriptorModelFactory.isInternalVirtualLink(d);
							return notOverSelf && (isInternalCP || isInternalVLD);

						}).classed('-is-valid-drop-target', true);

					}

					const validDropTarget = getContainerUnderMouse(comp, this, comp.graph.scale);
					comp.graph.g.selectAll('.-is-drag-over').classed('-is-drag-over', false);
					if (validDropTarget) {
						validDropTarget.classed('-is-drag-over', true);
					}

				};

				// determine what the interaction is and do it
				comp.boundMouseUpHandler = function () {

					// remove these handlers so they start fresh on the next drag operation
					comp.graph.svg
						.on('mouseup.edgeBuilder', null)
						.on('mousemove.edgeBuilder', null);

					// remove visual treatments
					comp.graph.svg.classed('-is-dragging-connection-point', false);
					comp.descriptors().classed('-is-valid-drop-target', false);
					comp.connectionPoints().classed('-is-valid-drop-target', false);
					comp.graph.g.selectAll('.-is-drag-over').classed('-is-drag-over', false);

					// determine if there is a connection point
					const dstConnectionPoint = comp.connectionPoints().filter(d => {
						if (/connection-point/.test(d.type)) {
							return mouseWithinPosition(d.position, d3.mouse(this), comp.graph.scale);
						}
					});
					if (dstConnectionPoint[0].length) {
						const src = srcConnectionPoint.datum();
						const dst = dstConnectionPoint.datum();
						if (src.key !== dst.key) {
							// do not drop on self
							if (DescriptorGraphPathBuilder.addConnection(srcConnectionPoint, dstConnectionPoint)) {
								existingPath.remove();
							}
						}
					} else {
						// otherwise determine if destination is a VLD
						const dstVirtualLink = comp.descriptors().filter(d => {
							return /vld/.test(d.type) && mouseWithinPosition(d.position, d3.mouse(this), comp.graph.scale);
						});
						if (dstVirtualLink[0].length) {
							if (DescriptorGraphPathBuilder.addConnectionToVLD(srcConnectionPoint, dstVirtualLink)) {
								existingPath.remove();
							}
						}
					}

					// if we hid an existing path restore it
					existingPath.style({
						opacity: null
					});

					// remove the tracer path
					path.remove();

				};

				// init drag handlers
				comp.graph.svg
					.on('mouseup.edgeBuilder', comp.boundMouseUpHandler)
					.on('mousemove.edgeBuilder', comp.boundMouseMoveHandler);
			}

		};

		// enable dragging features
		comp.graph.svg
			.on('mousedown.edgeBuilder', comp.boundMouseDownHandler);

	}

}
