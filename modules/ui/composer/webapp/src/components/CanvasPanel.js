
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
'use strict';

import _ from 'lodash'
import React from 'react'
import PureRenderMixin from 'react-addons-pure-render-mixin'
import utils from '../libraries/utils'
import messages from './messages'
import DescriptorModelFactory from '../libraries/model/DescriptorModelFactory'
import CatalogItemCanvasEditor from './CatalogItemCanvasEditor'
import CatalogItemsActions from '../actions/CatalogItemsActions'
import CanvasEditorActions from '../actions/CanvasEditorActions'
import ComposerAppActions from '../actions/ComposerAppActions'
import CanvasZoom from './CanvasZoom'
import CanvasPanelTray from './CanvasPanelTray'
import ForwardingGraphCanvasEditor from './ForwardingGraphPathsEditor'
import SelectionManager from '../libraries/SelectionManager'

import '../styles/CanvasPanel.scss'

const CanvasPanel = React.createClass({
	mixins: [PureRenderMixin],
	getInitialState() {
		return {};
	},
	getDefaultProps() {
		return {
			title: '',
			layout: {
				left: 300,
				right: 300
			},
			showMore: false,
			containers: []
		};
	},
	componentWillMount() {
	},
	componentDidMount() {
		SelectionManager.addEventListeners();
	},
	componentDidUpdate() {
	},
	componentWillUnmount() {
		SelectionManager.removeEventListeners();
	},
	render() {
		var style = {
			left: this.props.layout.left
		};
		const hasItem = this.props.containers.length !== 0;
		const isEditingNSD = DescriptorModelFactory.isNetworkService(this.props.containers[0]);
		const hasNoCatalogs = this.props.hasNoCatalogs;
		const bodyComponent = hasItem ? <CatalogItemCanvasEditor zoom={this.props.zoom} isShowingMoreInfo={this.props.showMore} containers={this.props.containers}/> : messages.canvasWelcome();
		return (
			<div className="CanvasPanel" style={style} onDragOver={this.onDragOver} onDrop={this.onDrop}>
				<div className="CanvasPanelHeader panel-header" data-resizable="limit_bottom">
					<h1>
						<span className="model-name">{this.props.title}</span>
					</h1>
				</div>
				<div className="CanvasPanelBody panel-body" style={{marginRight: this.props.layout.right, bottom: this.props.layout.bottom}} >
					{hasNoCatalogs ? null : bodyComponent}
				</div>
				<CanvasZoom zoom={this.props.zoom} style={{bottom: this.props.layout.bottom + 20}}/>
				<CanvasPanelTray layout={this.props.layout} show={isEditingNSD}>
					<ForwardingGraphCanvasEditor containers={this.props.containers} />
				</CanvasPanelTray>
			</div>
		);
	},
	onDragOver(event) {
		const isDraggingFiles = _.contains(event.dataTransfer.types, 'Files');
		if (!isDraggingFiles) {
			event.preventDefault();
			event.dataTransfer.dropEffect = 'copy';
		}
	},
	onDrop(event) {
		// given a drop event determine which action to take in the canvas:
		// open item or add item to an existing, already opened nsd
		// note: nsd is the only editable container
		const data = utils.parseJSONIgnoreErrors(event.dataTransfer.getData('text'));
		if (data.type === 'catalog-item') {
			this.handleDropCatalogItem(event, data);
		} else if (data.type === 'action') {
			this.handleDropCanvasAction(event, data);
		}
	},
	handleDropCanvasAction(event, data) {
		if (data.action === 'add-vld') {
			event.preventDefault();
			this.addVirtualLink({clientX: event.clientX, clientY: event.clientY});
		} else if (data.action === 'add-vnffgd') {
			event.preventDefault();
			this.addForwardingGraph({clientX: event.clientX, clientY: event.clientY});
		}
	},
	handleDropCatalogItem(event, data) {
		let openItem = null;
		const currentItem = this.props.containers[0];
		if (data.item.uiState.type === 'nsd') {
			// if item is an nsd then open the descriptor in the canvas
			openItem = data.item;
			// if item is a vnfd or pnfd then check if the current item is an nsd
		} else if (DescriptorModelFactory.isNetworkService(currentItem)) {
			// so add the item to the nsd and re-render the canvas
			switch (data.item.uiState.type) {
			case 'vnfd':
				this.addVirtualNetworkFunction(data.item, {clientX: event.clientX, clientY: event.clientY});
				break;
			case 'pnfd':
				this.addPhysicalNetworkFunction(data.item, {clientX: event.clientX, clientY: event.clientY});
				break;
			default:
				console.warn(`Unknown catalog-item type. Expect type "nsd", "vnfd" or "pnfd" but got ${data.item.uiState.type}.`);
			}
		} else {
			// otherwise the default action is to open the item
			openItem = data.item;
		}
		if (openItem) {
			event.preventDefault();
			CatalogItemsActions.editCatalogItem(openItem);
		}
	},
	addVirtualLink(dropCoordinates) {
		const currentItem = this.props.containers[0];
		if (DescriptorModelFactory.isNetworkService(currentItem)) {
			const vld = currentItem.createVld();
			vld.uiState.dropCoordinates = dropCoordinates;
			CatalogItemsActions.catalogItemDescriptorChanged(currentItem);
		}
	},
	addForwardingGraph(dropCoordinates) {
		const currentItem = this.props.containers[0];
		if (DescriptorModelFactory.isNetworkService(currentItem)) {
			const vld = currentItem.createVnffgd();
			vld.uiState.dropCoordinates = dropCoordinates;
			CatalogItemsActions.catalogItemDescriptorChanged(currentItem);
		}
	},
	addVirtualNetworkFunction(model, dropCoordinates) {
		const currentItem = this.props.containers[0];
		if (DescriptorModelFactory.isNetworkService(currentItem)) {
			const vnfd = DescriptorModelFactory.newVirtualNetworkFunction(model);
			const cvnfd = currentItem.createConstituentVnfdForVnfd(vnfd);
			cvnfd.uiState.dropCoordinates = dropCoordinates;
			CatalogItemsActions.catalogItemDescriptorChanged(currentItem);
		}
	},
	addPhysicalNetworkFunction(model, dropCoordinates) {
		const currentItem = this.props.containers[0];
		if (DescriptorModelFactory.isNetworkService(currentItem)) {
			const pnfd = DescriptorModelFactory.newPhysicalNetworkFunction(model);
			pnfd.uiState.dropCoordinates = dropCoordinates;
			currentItem.createPnfd(pnfd);
			CatalogItemsActions.catalogItemDescriptorChanged(currentItem);
		}
	}
});

export default CanvasPanel;
