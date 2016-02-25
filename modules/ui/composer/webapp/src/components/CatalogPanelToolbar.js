
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
'use strict';

import React from 'react'
import Button from './Button'
import PureRenderMixin from 'react-addons-pure-render-mixin'
import CatalogPanelTrayActions from '../actions/CatalogPanelTrayActions'
import CatalogItemsActions from '../actions/CatalogItemsActions'

import '../styles/CatalogPanelToolbar.scss'

import imgAdd from '../images/add175.svg'
import imgCopy from '../images/copy30.svg'
import imgOnboard from '../images/upload109.svg'
import imgUpdate from '../images/file91.svg'
import imgDownload from '../images/download158.svg'
import imgDelete from '../images/recycle69.svg'

const CatalogHeader = React.createClass({
	mixins: [PureRenderMixin],
	getInitialState() {
		return {};
	},
	getDefaultProps() {
	},
	componentWillMount() {
	},
	componentDidMount() {
	},
	componentDidUpdate() {
	},
	componentWillUnmount() {
	},
	render() {
		return (
			<div className="CatalogPanelToolbar">
				<h1>Descriptor Catalogs</h1>
				<div className="btn-bar">
					<div className="btn-group">
						<Button type="image" title="OnBoard a catalog package" className="action-onboard-catalog-package" onClick={this.onClickOnBoardCatalog} src={imgOnboard} />
						<Button type="image" title="Update a catalog package" className="action-update-catalog-package" onClick={this.onClickUpdateCatalog} src={imgUpdate} />
						<Button type="image" title="Export selected catalog item(s)" className="action-export-catalog-items" onClick={this.onClickExportCatalogItems} src={imgDownload} />
					</div>
					<div className="btn-group">
						<div className="menu">
							<Button type="image" title="Create a new catalog item" className="action-create-catalog-item" onClick={this.onClickCreateCatalogItem.bind(null, 'nsd')} src={imgAdd} />
							<div className="sub-menu">
								<Button type="image" title="Create a new catalog item" className="action-create-catalog-item" onClick={this.onClickCreateCatalogItem.bind(null, 'nsd')} src={imgAdd} label="Add NSD" />
								<Button type="image" title="Create a new catalog item" className="action-create-catalog-item" onClick={this.onClickCreateCatalogItem.bind(null, 'vnfd')} src={imgAdd} label="Add VNFD" />
							</div>
						</div>
						<Button type="image" title="Copy catalog item" className="action-copy-catalog-item" onClick={this.onClickDuplicateCatalogItem} src={imgCopy} />
					</div>
					<div className="btn-group">
						<Button type="image" title="Delete catalog item" className="action-delete-catalog-item" onClick = {this.onClickDeleteCatalogItem} src={imgDelete} />
					</div>
				</div>
			</div>
		);
	},
	onClickUpdateCatalog() {
		//CatalogPanelTrayActions.open();
		// note CatalogPackageManagerUploadDropZone wired our btn
		// click event to the DropZone.js configuration and will
		// open the tray when/if files are added to the drop zone
	},
	onClickOnBoardCatalog() {
		//CatalogPanelTrayActions.open();
		// note CatalogPackageManagerUploadDropZone wired our btn
		// click event to the DropZone.js configuration and will
		// open the tray when/if files are added to the drop zone
	},
	onClickDeleteCatalogItem() {
		CatalogItemsActions.deleteSelectedCatalogItem();
	},
	onClickCreateCatalogItem(type) {
		CatalogItemsActions.createCatalogItem(type);
	},
	onClickDuplicateCatalogItem() {
		CatalogItemsActions.duplicateSelectedCatalogItem();
	},
	onClickExportCatalogItems() {
		CatalogPanelTrayActions.open();
		CatalogItemsActions.exportSelectedCatalogItems();
	}
});

export default CatalogHeader;
