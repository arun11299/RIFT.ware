/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
/**
 * Created by onvelocity on 2/4/16.
 */

'use strict';

import React from 'react'
import Range from './Range'
import Button from './Button'
import ClassNames from 'classnames'
import changeCase from 'change-case'
import LayoutRow from './LayoutRow'
import SelectionManager from '../libraries/SelectionManager'
import PureRenderMixin from 'react-addons-pure-render-mixin'
import CatalogItemsActions from '../actions/CatalogItemsActions'
import CanvasEditorActions from '../actions/CanvasEditorActions'
import DescriptorModelFactory from '../libraries/model/DescriptorModelFactory'
import ComposerAppActions from '../actions/ComposerAppActions'
import DescriptorModelMetaFactory from '../libraries/model/DescriptorModelMetaFactory'
import ComposerAppStore from '../stores/ComposerAppStore'
import DeletionManager from '../libraries/DeletionManager'
import ContentEditableDiv from './ContentEditableDiv'

import '../styles/ForwardingGraphPathsEditor.scss'
import imgNSD from '../images/iconmonstr-network-6-icon.svg'
import imgFG from '../images/iconmonstr-infinity-4-icon.svg'
import imgRemove from '../images/recycle69.svg'
import imgAdd from '../images/iconmonstr-plus-5-icon-256.png'
import imgConnection from '../images/connection.svg'
import imgClassifier from '../images/iconmonstr-control-panel-4.svg'
import imgReorder from '../images/menu51.svg'

function onHoverHighlightConnectionPoint(cpNumber, event) {
	const found = Array.from(document.querySelectorAll('[data-cp-number="' + cpNumber + '"]'));
	if (event.type === 'mouseenter') {
		found.forEach(d => d.classList.add('-is-highlight'));
	} else {
		found.forEach(d => d.classList.remove('-is-highlight'));
	}
}

function onClickSelectAndShowInDetailsPanel(container, event) {
	if (event.defaultPrevented) return;
	event.preventDefault();
	if (container.isFactory) {
		return
	}
	ComposerAppActions.selectModel(container);
	ComposerAppActions.outlineModel.defer(container);
}

function onCutRemove(cp, event) {
	if (event.defaultPrevented) return;
	if (cp.remove()) {
		CatalogItemsActions.catalogItemDescriptorChanged.defer(cp.getRoot());
	} else {
		event.preventDefault();
	}
	event.stopPropagation();
}

function mapCP(cpRef, i) {

	const context = this;

	return (
		<div key={cpRef.uid} className="rsp">
			<div className="rsp-line" style={context.stylePrimary}></div>
			<div className={cpRef.className} data-uid={cpRef.uid} data-datum={cpRef} style={context.stylePrimary}
				 onCut={onCutRemove.bind(null, cpRef)}
				 onMouseEnter={onHoverHighlightConnectionPoint.bind(null, cpRef.cpNumber)}
				 onMouseLeave={onHoverHighlightConnectionPoint.bind(null, cpRef.cpNumber)}
				 onClick={onClickSelectAndShowInDetailsPanel.bind(null, cpRef)}>
				<small>{cpRef.cpNumber}</small>
			</div>
		</div>
	);

}

function mapConnector(context, rsp, connector, i) {
	function onClickAddConnectionPointRef(rsp, connector, event) {
		event.preventDefault();
		if (rsp.isFactory) {
			const newRsp = rsp.createVnfdConnectionPointRef(connector);
			context.component.setState({editPathsMode: newRsp.uid});
		} else {
			rsp.createVnfdConnectionPointRef(connector);
		}
		CatalogItemsActions.catalogItemDescriptorChanged(rsp.getRoot());
	}
	const cpNumber = connector.cpNumber;
	return (
		<div key={connector.uid} className={connector.className} style={context.styleSecondary}
			 onClick={onClickAddConnectionPointRef.bind(null, rsp, connector)}
			 onMouseEnter={onHoverHighlightConnectionPoint.bind(null, cpNumber)}
			 onMouseLeave={onHoverHighlightConnectionPoint.bind(null, cpNumber)}><small>cp{cpNumber}</small></div>
	);
}

function mapNextValidConnectionPoint(context, rsp, cvnfd, i) {

	//const context = this;

	return (
		<div key={i} className={cvnfd.className}>
			<small className="vnfd-title">{cvnfd.title}</small>
			<div className="connectors">
				{cvnfd.connectors.map(mapConnector.bind(null, context, rsp))}
			</div>
		</div>
	);

}

function removeHighlighting(rsp) {
	Array.from(document.querySelectorAll(`svg .forwarding-graph-paths`)).forEach(d => {
		d.classList.remove('-is-highlighting');
	});
	Array.from(document.querySelectorAll('.rsp-path')).forEach(d => {
		d.classList.remove('-is-highlighted');
	});
}

function highlightPath(rsp) {
	removeHighlighting(rsp);
	//
	Array.from(document.querySelectorAll(`svg .forwarding-graph-paths`)).forEach(d => {
		d.classList.add('-is-highlighting');
	});
	Array.from(document.querySelectorAll(`[data-id="${rsp.id}"]`)).forEach(d => {
		d.classList.add('-is-highlighted');
	});
}

function mapRSP(rsp, i) {

	const context = this;
	context.rsp = rsp;

	rsp.uiState.showPath = rsp.uiState.hasOwnProperty('showPath') ? rsp.uiState.showPath : true;

	function onClickAddConnectionPointRef(rsp, event) {
		event.preventDefault();
		const ref = rsp.createVnfdConnectionPointRef();
		CatalogItemsActions.catalogItemDescriptorChanged(ref.getRoot());
	}

	function onClickRemoveRecordServicePath(rsp, event) {
		event.preventDefault();
		const root = rsp.getRoot();
		rsp.remove();
		CatalogItemsActions.catalogItemDescriptorChanged(root);
	}

	function onClickCreateRecordServicePath(rsp, event) {
		event.preventDefault();
		rsp.create();
		this.component.setState({editPathsMode: rsp.uid});
		CatalogItemsActions.catalogItemDescriptorChanged(rsp.getRoot());
	}

	function onClickEnterPathEdithMode(rsp, event) {
		event.preventDefault();
		this.setState({editPathsMode: rsp.uid});
	}

	function onClickExitPathEdithMode(rsp, event) {
		event.preventDefault();
		this.setState({editPathsMode: false});
	}

	function onClickToggleRSPShowPath(rsp, event) {
		// warn preventing default will undo the user's action
		//event.preventDefault();
		rsp.uiState.showPath = event.target.checked;
		rsp.parent.uiState.showPaths = rsp.parent.rsp.filter(rsp => rsp.uiState.showPath === true).length === rsp.parent.rsp.length;
		CatalogItemsActions.catalogItemMetaDataChanged(rsp.getRoot().model);
	}

	function onClickToggleHideShowClassifiers(context, rsp, event) {
		event.preventDefault();
		const state = context.component.state.showClassifiers || {};
		rsp.uiState.showClassifiers = state[rsp.uid] = !state[rsp.uid];
		context.component.setState({showClassifiers: state});
		CatalogItemsActions.catalogItemMetaDataChanged(rsp.getRoot().model);
	}

	function onInputUpdateModel(context, attr, name, event) {
		event.preventDefault();
		attr.setFieldValue(name, event.target.value);
		CatalogItemsActions.catalogItemDescriptorChanged(attr.getRoot());
	}

	function onClickMatchAttributeFieldValue(context, attr, name, event) {
		const isSelected = SelectionManager.isSelected(attr.parent);
		if (!isSelected) {
			// let the click through so it selects the row
		} else {
			event.preventDefault();
		}
	}

	function onFocusMatchAttributeField(attr, event) {
		DeletionManager.removeEventListeners();
		ComposerAppActions.selectModel(attr.parent);
		ComposerAppActions.outlineModel.defer(attr.parent);
	}

	function onClickCreateClassifier(rsp, event) {
		event.preventDefault();
		rsp.createClassifier();
		CatalogItemsActions.catalogItemDescriptorChanged(rsp.getRoot());
	}

	function mapClassifierMatchAttributes(context, matchAttributes) {
		return matchAttributes.fieldNames.map((name, i) => {
			return (
				<td key={i} className={matchAttributes.className}>
					<div className="match-attr-name">{name}</div>
					<ContentEditableDiv value={matchAttributes.getFieldValue(name)}
										onClick={onClickMatchAttributeFieldValue.bind(null, context, matchAttributes, name)}
										onFocus={onFocusMatchAttributeField.bind(null, matchAttributes)}
										onBlur={() => DeletionManager.addEventListeners()}
										onChange={onInputUpdateModel.bind(null, context, matchAttributes, name)}
										className="match-attr-value"/>
				</td>
			);
		});
	}

	function mapClassifiers(context, rsp, classifier, i) {

		return (
			<tr key={classifier.uid} data-uid={classifier.uid} data-datum={classifier} onCut={onCutRemove.bind(null, classifier)} className={classifier.className} onClick={onClickSelectAndShowInDetailsPanel.bind(null, classifier)}>
				<th key="primary-action-column" className="primary-action-column"><img src={imgReorder}/></th><th key="secondary-action-column" className="secondary-action-column"> </th>
				{mapClassifierMatchAttributes(context, classifier.matchAttributes)}
			</tr>
		);

	}

	const isEditPathsMode = context.component.state.editPathsMode === rsp.uid;

	const toggleSelectionOrCreateNewPath = (
		<div>
			{!rsp.isFactory ? <input type="checkbox" id={'show-path-' + rsp.uid} checked={rsp.uiState.showPath} defaultValue="true" onChange={onClickToggleRSPShowPath.bind(null, rsp)} /> : ' '}
		</div>
	);

	const toggleClassifiersEditor = (
		<Button src={imgClassifier} title="Show Classifier rules for this RSP" onClick={onClickToggleHideShowClassifiers.bind(null, context, rsp)} />
	);

	const attributeNames = DescriptorModelMetaFactory.getModelFieldNamesForType('nsd.vnffgd.classifier.match-attributes');

	const showClassifiers = context.component.state.showClassifiers[rsp.uid];

	return (
		<div key={i} className="rsp" data-offset-width="table.rsp-classifier" data-uid={rsp.uid} data-datum={rsp}
			 onClick={onClickSelectAndShowInDetailsPanel.bind(null, rsp)}
			 onMouseOver={highlightPath.bind(null, rsp)}
			 onMouseOut={removeHighlighting.bind(null, rsp)}
			 onMouseLeave={removeHighlighting.bind(null, rsp)}
			 onCut={onCutRemove.bind(null, rsp)}>
			<div className={ClassNames(rsp.className, {'-is-factory': rsp.isFactory, '-is-edit-paths-mode': isEditPathsMode})}>
				<div className="header-actions">
					{!rsp.isFactory ? <Button className="remove-record-service-path" title="Remove"
											  onClick={onClickRemoveRecordServicePath.bind(null, rsp)}
											  src={imgRemove}/> : null}
				</div>
				<LayoutRow primaryActionColumn={toggleSelectionOrCreateNewPath}
						   secondaryActionColumn={!rsp.isFactory ? toggleClassifiersEditor : null}>
					<div className="connection-points">
						{rsp.connectionPoints.map(mapCP.bind(context))}
						<div className="rsp-create-new-connection-point-line rsp-line"
							 style={context.styleSecondary}></div>
						<div className="enter-path-edit-mode ConnectionPoint" style={context.styleSecondary}
							 onClick={onClickEnterPathEdithMode.bind(context.component, rsp)}>
							<small>+CP</small>
						</div>
						<div className="selection">
							<div className="select-next-connection-point-list">
								{context.containers.filter(d => DescriptorModelFactory.isConstituentVnfd(d)).map(mapNextValidConnectionPoint.bind(null, context, rsp))}
								<small className="Button"
									   onClick={onClickExitPathEdithMode.bind(context.component, rsp)}>done
								</small>
							</div>
						</div>
						{rsp.isFactory && !isEditPathsMode? <small className="enter-path-edit-mode-hint hint">Tap to start creating a new path.</small> : null}
					</div>
				</LayoutRow>
			</div>
			<table className={ClassNames('rsp-classifier', {'-is-show-classifiers': showClassifiers})}>
				<thead>
					<tr>
						<th key="primary-action-column" className="primary-action-column"><div className="primary-action-column"></div></th><th key="secondary-action-column" className="secondary-action-column"><div className="secondary-action-column"></div></th>
						{attributeNames.map((name, i) => <th key={i}>{changeCase.title(name)}</th>)}
					</tr>
				</thead>
				<tbody>
					{!rsp.isFactory ? rsp.classifier.map(mapClassifiers.bind(null, context, rsp)) : null}
				</tbody>
				<tfoot>
					<tr>
						<th colSpan={attributeNames.length + 2}>
							<Button className="create-new-classifier" src={imgAdd} width="20px" onClick={onClickCreateClassifier.bind(null, rsp)} label="" />
							<small className="create-new-classifier action hint" onClick={onClickCreateClassifier.bind(null, rsp)}>Add new Classifier</small>
						</th>
					</tr>
				</tfoot>
			</table>
		</div>
	);

}

function mapFG(fg, i) {

	const context = this;
	context.vnffg = fg;

	const colors = fg.colors;
	const stylePrimary = {borderColor: colors.primary};
	const styleSecondary = {borderColor: colors.secondary};

	context.stylePrimary = stylePrimary;
	context.styleSecondary = styleSecondary;

	const rspMap = fg.rsp.reduce((map, rsp) => {
		map[rsp.id] = rsp;
		rsp.classifier = [];
		return map;
	}, {});

	fg.classifier.forEach(classifier => {
		const rsp = rspMap[classifier.model['rsp-id-ref']];
		if (rsp) {
			rsp.classifier.push(classifier);
		}
	});

	function onClickRemoveForwardingGraph(fg, event) {
		event.preventDefault();
		const root = fg.getRoot();
		fg.remove();
		CatalogItemsActions.catalogItemDescriptorChanged(root);
	}

	function onClickAddRecordServicePath(fg, event) {
		event.preventDefault();
		fg.createRsp();
		CatalogItemsActions.catalogItemDescriptorChanged(fg.getRoot());
	}

	function onClickToggleShowAllFGPaths(fg, event) {
		//event.preventDefault();
		fg.uiState.showPaths = event.target.checked;
		fg.rsp.forEach(rsp => rsp.uiState.showPath = event.target.checked);
		CatalogItemsActions.catalogItemMetaDataChanged(fg.getRoot().model);
	}

	if (!fg.uiState.hasOwnProperty('showPaths')) {
		fg.uiState.showPaths = true;
		fg.rsp.forEach(d => d.uiState.showPath = true);
	}

	const toggleSelectAllPaths = (
		<input type="checkbox" name={'show-path' + fg.uid} checked={fg.uiState.showPaths} onChange={onClickToggleShowAllFGPaths.bind(null, fg)} />
	);

	const srpFactory = DescriptorModelFactory.newRecordServicePathFactory({}, fg);
	srpFactory.uid = fg.uid + i;

	return (
		<div key={i} className={fg.className} data-uid={fg.uid} data-datum={fg} data-offset-width="table.rsp-classifier" onClick={onClickSelectAndShowInDetailsPanel.bind(null, fg)} onCut={onCutRemove.bind(null, fg)}>
			<div key="outline-indicator" data-outline-indicator="true"></div>
			<div className="header-actions">
				<Button className="remove-forwarding-graph" title="Remove" onClick={onClickRemoveForwardingGraph.bind(null, fg)} src={imgRemove}/>
			</div>
			<LayoutRow primaryActionColumn={toggleSelectAllPaths} secondaryActionColumn={<img className="fg-icon" src={imgFG} width="20px" />}>
				<small>{fg.title}</small>
			</LayoutRow>
			{fg.recordServicePaths.concat(srpFactory).map(mapRSP.bind(context))}
		</div>
	);

}

function mapNSD(nsd, i) {

	const context = this;
	context.nsd = nsd;

	function onClickAddForwardingGraph(nsd, event) {
		event.preventDefault();
		nsd.createVnffgd();
		CatalogItemsActions.catalogItemDescriptorChanged(nsd.getRoot());
	}

	const forwardingGraphs = nsd.forwardingGraphs.map(mapFG.bind(context));
	if (forwardingGraphs.length === 0) {
		forwardingGraphs.push(
			<div key="1" className="welcome-message">
				No Forwarding Graphs to model.
			</div>
		);
	}

	return (
		<div key={i} className={nsd.className}>
			{forwardingGraphs}
			<div className="footer-actions">
				<div className="row-action-column">
					<Button className="create-new-forwarding-graph" src={imgAdd} width="20px" onClick={onClickAddForwardingGraph.bind(null, nsd)} label="" />
					<small className="create-new-forwarding-graph action hint" onClick={onClickAddForwardingGraph.bind(null, nsd)}>Add new Forwarding Graph</small>
				</div>
			</div>
		</div>
	);

}

const ForwardingGraphPathsEditor = React.createClass({
	mixins: [PureRenderMixin],
	getInitialState: function () {
		return ComposerAppStore.getState();
	},
	getDefaultProps: function () {
		return {
			containers: []
		};
	},
	componentWillMount: function () {
	},
	componentDidMount: function () {
	},
	componentDidUpdate: function () {
	},
	componentWillUnmount: function () {
	},
	render() {

		const containers = this.props.containers;
		const context = {
			component: this,
			containers: containers
		};

		const networkService = containers.filter(d => d.type === 'nsd');
		if (networkService.length === 0) {
			return <p className="welcome-message">No <img src={imgNSD} width="20px" /> NSD open in the canvas. Try opening an NSD.</p>;
		}

		return (
			<div className="ForwardingGraphPathsEditor -with-transitions" data-offset-parent="true">
				<div key="outline-indicator" data-outline-indicator="true"></div>
				{containers.filter(d => d.type === 'nsd').map(mapNSD.bind(context))}
			</div>
		);

	}
});

export default ForwardingGraphPathsEditor;
