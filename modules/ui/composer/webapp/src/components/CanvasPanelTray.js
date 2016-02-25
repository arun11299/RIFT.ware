/**
 * Created by onvelocity on 2/4/16.
 */
'use strict';
import React from 'react'
import ClassNames from 'classnames'
import SelectionManager from '../libraries/SelectionManager'
import CanvasPanelTrayActions from '../actions/CanvasPanelTrayActions'
import '../styles/CanvasPanelTray.scss'
export default function (props) {
	const style = {
		height: Math.max(0, props.layout.bottom),
		right: props.layout.right,
		display: props.show ? false : 'none'
	};
	const classNames = ClassNames('CanvasPanelTray', {'-with-transitions': !document.body.classList.contains('resizing')});
	function pauseSelectionManager(event) {
		event.preventDefault();
		SelectionManager.pause();
	}
	return (
		<div className={classNames} data-resizable="top" style={style}>
			<h1 onClick={CanvasPanelTrayActions.toggleOpenClose} onMouseDown={pauseSelectionManager} onMouseOver={SelectionManager.pause} onMouseOut={SelectionManager.resume} onMouseLeave={SelectionManager.resume}>Forwarding Graphs</h1>
			<div className="tray-body">
				{props.children}
			</div>
		</div>
	);
}