/**
 * Created by onvelocity on 2/13/16.
 */
'use static';
import React from 'react'
export default function ContentEditableDiv (props) {

	return (
		<input className="ContentEditableDiv" {...props} style={{borderColor: 'transparent', background: 'transparent'}} />
	);

}