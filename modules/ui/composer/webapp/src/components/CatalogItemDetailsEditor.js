/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
'use strict';

import utils from '../libraries/utils'
import changeCase from 'change-case'
import ClassNames from 'classnames'
import React from 'react'
import PureRenderMixin from 'react-addons-pure-render-mixin'
import CatalogItemsActions from '../actions/CatalogItemsActions'
import buildDescriptorModelFormEditor from '../libraries/model/DescriptorModelFormEditor'

import '../styles/CatalogItemDetailsEditor.scss'

const CatalogItemDetailsEditor = React.createClass({
	mixins: [PureRenderMixin],
	getInitialState() {
		return {
			html: ''
		};
	},
	getDefaultProps() {
		return {
			container: null,
			width: 0
		};
	},
	componentWillMount() {
	},
	componentDidMount() {
	},
	componentDidUpdate() {
	},
	componentWillUnmount() {
	},
	notifyCatalogItemChanged(item) {
		CatalogItemsActions.catalogItemDescriptorChanged(item);
	},
	render() {

		const container = this.props.container || {model: {}, uiState: {}};
		if (!(container && container.model && container.uiState)) {
			return null;
		}

		const props = {
			container: this.props.container,
			width: this.props.width
		};

		return (
			<div className="CatalogItemDetailsEditor">
				<form name="details-descriptor-editor-form">
					<div className="properties-group">
						{buildDescriptorModelFormEditor(props)}
					</div>
				</form>
			</div>
		);

	}
});

export default CatalogItemDetailsEditor;
