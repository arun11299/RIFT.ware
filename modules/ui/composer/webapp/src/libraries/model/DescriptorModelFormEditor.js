/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
/**
 * Created by onvelocity on 1/18/16.
 *
 * This class generates the form fields used to edit the CONFD JSON model.
 */
'use strict';

import alt from '../../alt'
import _ from 'lodash'
import utils from './../utils'
import React from 'react'
import ClassNames from 'classnames'
import changeCase from 'change-case'
import DescriptorModel from './DescriptorModel'
import toggle from './../ToggleElementHandler'
import Button from '../../components/Button'
import Property from './DescriptorModelMetaProperty'
import ComposerAppActions from '../../actions/ComposerAppActions'
import CatalogItemsActions from '../../actions/CatalogItemsActions'
import DESCRIPTOR_MODEL_FIELDS from './DescriptorModelFields'
import DescriptorModelMetaFactory from './DescriptorModelMetaFactory'
import SelectionManager from '../SelectionManager'
import DeletionManager from '../DeletionManager'

import imgAdd from '../../images/add175.svg'
import imgRemove from '../../images/recycle69.svg'

function getDescriptorMetaBasicForType(type) {
	const basicPropertiesFilter = d => _.contains(DESCRIPTOR_MODEL_FIELDS[type], d.name);
	return DescriptorModelMetaFactory.getModelMetaForType(type, basicPropertiesFilter) || {properties: []};
}

function getDescriptorMetaAdvancedForType(type) {
	const advPropertiesFilter = d => !_.contains(DESCRIPTOR_MODEL_FIELDS[type], d.name);
	return DescriptorModelMetaFactory.getModelMetaForType(type, advPropertiesFilter) || {properties: []};
}

export default function buildDescriptorModelFormEditor(props) {

	const container = props.container;

	if (!(container instanceof DescriptorModel)) {
		return null;
	}

	function startEditing() {
		DeletionManager.removeEventListeners();
	}

	function endEditing() {
		DeletionManager.addEventListeners();
	}

	function onFocusPropertyFormInputElement(property, path, value, event) {
		event.preventDefault();
		event.stopPropagation();
		startEditing();
		this.getRoot().uiState.focusedPropertyPath = path.join('.');
		console.log('property selected', path.join('.'));
		ComposerAppActions.propertySelected([path.join('.')]);
	}

	function buildAddPropertyAction(container, property, path) {
		function onClickAddProperty(property, path, event) {
			event.preventDefault();
			SelectionManager.resume();
			const create = Property.getContainerCreateMethod(property, this);
			if (create) {
				create(path, property);
			} else {
				const name = path.join('.');
				const value = Property.createModelInstance(property);
				utils.assignPathValue(this.model, name, value);
			}
			CatalogItemsActions.catalogItemDescriptorChanged(this.getRoot());
		}
		return (
				<Button className="inline-hint" onClick={onClickAddProperty.bind(container, property, path)} label="Add" src={imgAdd} />
		);
	}

	function buildRemovePropertyAction(container, property, path) {
		function onClickRemoveProperty(property, path, event) {
			event.preventDefault();
			const name = path.join('.');
			const removeMethod = Property.getContainerMethod(property, this, 'remove');
			if (removeMethod) {
				removeMethod(utils.resolvePath(this.model, name));
			} else {
				utils.removePathValue(this.model, name);
			}
			CatalogItemsActions.catalogItemDescriptorChanged(this.getRoot());
		}
		return (
			<div className="actions">
				<Button className="-remove-property" title="Remove" onClick={onClickRemoveProperty.bind(container, property, path)} src={imgRemove}/>
			</div>
		);
	}

	function onFormFieldValueChanged(event) {
		if (this instanceof DescriptorModel) {
			event.preventDefault();
			const name = event.target.name;
			const value = event.target.value;
			utils.assignPathValue(this.model, name, value);
			CatalogItemsActions.catalogItemDescriptorChanged(this.getRoot());
		}
	}

	function buildField(container, property, path, value, fieldKey) {

		const name = path.join('.');
		const isEditable = true;
		const onChange = onFormFieldValueChanged.bind(container);
		const isEnumeration = Property.isEnumeration(property);
		const onFocus = onFocusPropertyFormInputElement.bind(container, property, path, value);
		if (isEnumeration) {
			const enumeration = Property.getEnumeration(property, value);
			const options = enumeration.map((d, i) => {
				// note yangforge generates values for enums but the system does not use them
				// so we categorically ignore them
				// https://trello.com/c/uzEwVx6W/230-bug-enum-should-not-use-index-only-name
				//return <option key={fieldKey + ':' + i} value={d.value}>{d.name}</option>;
				return <option key={fieldKey + ':' + i} value={d.name}>{d.name}</option>;
			});
			const isValueSet = enumeration.filter(d => d.isSelected).length > 0;
			if (!isValueSet || property.cardinality === '0..1') {
				const noValueDisplayText = '';
				options.unshift(<option key={'(value-not-in-enum)' + fieldKey} value="">{noValueDisplayText}</option>);
			}
			return <select key={fieldKey} name={name} value={value} title={name} onChange={onChange} onFocus={onFocus} onBlur={endEditing} onMouseDown={startEditing} onMouseOver={startEditing} onMouseOut={SelectionManager.resume} onMouseLeave={SelectionManager.resume} readOnly={!isEditable}>{options}</select>;
		}

		if (property['preserve-line-breaks']) {
			return <textarea key={fieldKey} name={name} value={value} onChange={onChange} onFocus={onFocus} onBlur={endEditing} onMouseDown={startEditing} onMouseOver={startEditing} onMouseOut={endEditing} onMouseLeave={endEditing} readOnly={!isEditable} />;
		}

		return <input key={fieldKey} type="text" name={name} value={value} onChange={onChange} onFocus={onFocus} onBlur={endEditing} onMouseDown={startEditing} onMouseOver={startEditing} onMouseOut={endEditing} onMouseLeave={endEditing} readOnly={!isEditable}/>;

	}

	function buildElement(container, property, valuePath, value) {
		return property.properties.map((property, index) => {
			let childValue;
			const childPath = valuePath.slice();
			if (typeof value === 'object') {
				childValue = value[property.name];
			}
			childPath.push(property.name);

			return build(container, property, childPath, childValue);

		});
	}

	function buildChoice(container, property, path, value, key) {

		function onFormFieldValueChanged(event) {
			if (this instanceof DescriptorModel) {

				event.preventDefault();

				const name = event.target.name;
				const value = event.target.value;

				/*
					Transient State is stored for convenience in the uiState field.
					The choice yang type uses case elements to describe the "options".
					A choice can only ever have one option selected which allows
					the system to determine which type is selected by the name of
					the element contained within the field.
				 */

				//const stateExample = {
				//	uiState: {
				//		choice: {
				//			'conf-config': {
				//				selected: 'rest',
				//				'case': {
				//					rest: {},
				//					netconf: {},
				//					script: {}
				//				}
				//			}
				//		}
				//	}
				//};

				const statePath = ['uiState.choice'].concat(name);
				const stateObject = utils.resolvePath(this.model, statePath.join('.')) || {};
				// write state back to the model so the new state objects are captured
				utils.assignPathValue(this.model, statePath.join('.'), stateObject);

				// write the current choice value into the state
				const choiceObject = utils.resolvePath(this.model, [name, stateObject.selected].join('.'));
				if (choiceObject) {
					utils.assignPathValue(stateObject, ['case', stateObject.selected].join('.'), _.cloneDeep(choiceObject));
				}

				// remove the current choice value from the model
				utils.removePathValue(this.model, [name, stateObject.selected].join('.'));

				// get any state for the new selected choice
				const newChoiceObject = utils.resolvePath(stateObject, ['case', value].join('.')) || {};

				// assign new choice value to the model
				utils.assignPathValue(this.model, [name, value].join('.'), newChoiceObject);

				// update the selected name
				utils.assignPathValue(this.model, statePath.concat('selected').join('.'), value);

				CatalogItemsActions.catalogItemDescriptorChanged(this.getRoot());
			}
		}

		const caseByNameMap = {};

		const onChange = onFormFieldValueChanged.bind(container);

		const cases = property.properties.map(d => {
			if (d.type === 'case') {
				caseByNameMap[d.name] = d.properties[0];
				return {optionName: d.name, optionTitle: d.description};
			}
			caseByNameMap[d.name] = d;
			return {optionName: d.name};
		});

		const options = [{/*create a blank option*/}].concat(cases).map((d, i) => {
			return (
				<option key={i} value={d.optionName} title={d.optionTitle}>{d.optionName}</option>
			);
		});

		const selectName = path.join('.');
		const selectedOptionPath = ['uiState.choice', selectName, 'selected'].join('.');
		const selectedOptionValue = utils.resolvePath(container.model, selectedOptionPath);
		const valueProperty = caseByNameMap[selectedOptionValue] || {properties: []};

		const valueResponse = valueProperty.properties.map((d, i) => {
			const childPath = path.concat(valueProperty.name, d.name);
			const childValue = utils.resolvePath(container.model, childPath.join('.'));
			return (
				<div key={childPath.concat('index', i).join(':')}>
					{build(container, d, childPath, childValue, props)}
				</div>
			);
		});

		const onFocus = onFocusPropertyFormInputElement.bind(container, property, path, value);

		return (
			<div key={key} className="choice">
				<select key={Date.now()} name={selectName} value={selectedOptionValue} onChange={onChange} onFocus={onFocus} onBlur={endEditing} onMouseDown={startEditing} onMouseOver={startEditing} onMouseOut={endEditing} onMouseLeave={endEditing}>
					{options}
				</select>
				{valueResponse}
			</div>
		);

	}

	function buildListItem(container, property, path, value, key) {
		function onClickSelectItem(property, path, value, event) {
			event.preventDefault();
			const root = this.getRoot();
			SelectionManager.resume();
			if (SelectionManager.select(value)) {
				CatalogItemsActions.catalogItemMetaDataChanged(root.model);
			}
			SelectionManager.refreshOutline();
		}
		// todo need to abstract this better
		const title = value && (value.name || (value.uiState && value.uiState.displayName));
		return (
			<div key={Date.now()} className="simple-list-item" onClick={onClickSelectItem.bind(container, property, path, value)}>{title}</div>
		);
	}

	function build(container, property, path, value, props = {}) {

		const fields = [];
		const isLeaf = Property.isLeaf(property);
		const isArray = Property.isArray(property);
		const isObject = Property.isObject(property);
		const fieldKey = [container.id].concat(path);
		const isRequired = Property.isRequired(property);
		const title = changeCase.titleCase(property.name);
		const columnCount = property.properties.length || 1;
		const isColumnar = isArray && (Math.round(props.width / columnCount) > 155);
		const classNames = {'-is-required': isRequired, '-is-columnar': isColumnar};

		if (!property.properties && isObject) {
			const uiState = DescriptorModelMetaFactory.getModelMetaForType(property.name) || {};
			property.properties = uiState.properties;
		}

		const hasProperties = _.isArray(property.properties) && property.properties.length;
		const isMissingDescriptorMeta = !hasProperties && property.type !== 'leaf';

		// ensure value is not undefined for non-leaf property types
		if (isObject) {
			if (typeof value !== 'object') {
				value = isArray ? [] : {};
			}
		}
		const valueAsArray = _.isArray(value) ? value : [value];
		const titleInfo = isArray ? valueAsArray.filter(d => d).length + ' items' : '';

		const isMetaField = property.name === 'meta';
		const isCVNFD = property.name === 'constituent-vnfd';
		const isSimpleListView = Property.isSimpleList(property);
		const showAddAction = isArray && !isSimpleListView;

		valueAsArray.forEach((value, index) => {

			let field;
			const valuePath = path.slice();
			const key = fieldKey.concat([index]).join(':');

			if (isArray) {
				valuePath.push(index);
			}

			if (isMetaField) {
				if (typeof value === 'object') {
					value = JSON.stringify(value, undefined, 12);
				} else if (typeof value !== 'string') {
					value = '{}';
				}
				property['preserve-line-breaks'] = true;
			}

			if (isMissingDescriptorMeta) {
				field = <small key={fieldKey.concat('warning', index).join(':')} className="warning">No Descriptor Meta for {property.name}</small>;
			} else if (property.type === 'choice') {
				field = buildChoice(container, property, valuePath, value, key);
			} else if (isSimpleListView) {
				field = buildListItem(container, property, valuePath, value, key);
			} else {
				field = hasProperties ?
					buildElement(container, property, valuePath, value, key) :
					buildField(container, property, valuePath, value, key);
			}

			function onClickLeaf(property, path, value, event) {
				if (event.isDefaultPrevented()) {
					return;
				}
				event.preventDefault();
				event.stopPropagation();
				this.getRoot().uiState.focusedPropertyPath = path.join('.');
				console.log('property selected', path.join('.'));
				ComposerAppActions.propertySelected([path.join('.')]);
			}

			const clickHandler = isLeaf ? onClickLeaf : () => {};

			fields.push(
				<div key={fieldKey.concat(['property-group', index]).join(':')} onClick={clickHandler.bind(container, property, valuePath, value)} className={ClassNames('property-group', {'simple-list': isSimpleListView})}>
					{field}
					{isArray ? buildRemovePropertyAction(container, property, valuePath, fieldKey.concat([index]).join(':')) : null}
				</div>
			);

		});

		classNames['col-' + columnCount] = isColumnar;

		if (property.type === 'choice') {
			value = utils.resolvePath(container.model, ['uiState.choice'].concat(path, 'selected').join('.'));
		}

		const isToggled = false;

		return (
			<div key={fieldKey.join(':')} className={ClassNames(property.type, classNames)}>
				<h2 className={ClassNames('label', {'-is-toggled': isToggled})} data-toggle={isToggled ? 'true' : 'false'} onClick={toggle}>
					{title}
					<small className="info">{titleInfo}</small>
					{isLeaf ? <small className="value">{value}</small> : null}
					{isArray ? buildAddPropertyAction(container, property, path.concat(valueAsArray.length)) : null}
				</h2>
				<div className="toggleable">
					<span className="description">{property.description}</span>
					{isCVNFD ? <small>Drag a VNFD from the Catalog to add more.</small> : null}
					{fields}
				</div>
			</div>
		);

	}

	const containerType = container.uiState['qualified-type'] || container.uiState.type;
	const basicProperties = getDescriptorMetaBasicForType(containerType).properties;

	function buildBasicGroup() {
		if (basicProperties.length === 0) {
			return null;
		}
		return (
			<div>
				<h1 data-toggle="false" onClick={toggle}>Basic</h1>
				<div className="basic-properties-group toggleable">
					{basicProperties.map(property => {
						const path = [property.name];
						const value = container.model[property.name];
						return build(container, property, path, value);
					})}
				</div>
			</div>
		);
	}

	function buildAdvancedGroup() {
		const properties = getDescriptorMetaAdvancedForType(containerType).properties;
		if (properties.length === 0) {
			return null;
		}
		const closeGroup = basicProperties.length > 0;
		return (
			<div>
				<h1 data-toggle={closeGroup} className={ClassNames({'-is-toggled': closeGroup})} onClick={toggle}>Advanced</h1>
				<div className="advanced-properties-group toggleable">
					{properties.map(property => {
						const path = [property.name];
						const value = container.model[property.name];
						return build(container, property, path, value, {toggle: true, width: props.width});
					})}
				</div>
			</div>
		);
	}

	return (
		<div className="DescriptorModelFormEditor">
			<h1><small>{container.title}</small></h1>
			{buildBasicGroup()}
			{buildAdvancedGroup()}
		</div>
	);

}
