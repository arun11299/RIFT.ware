
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
import React from 'react/addons';
import '../../../../node_modules/loaders.css/src/animations/line-scale-pulse-out-rapid.scss';
import './loading-indicator-animations.scss';
let ReactCSSTransitionGroup = React.addons.CSSTransitionGroup;
export default class Loader extends React.Component {
  constructor(props) {
    super(props);
  }
  render() {
    let loader = '';
    var style = {
      height: this.props.size + 'rem',
      width: this.props.size * 0.15 + 'rem',
      backgroundColor: this.props.color || 'white'
    }
    if (this.props.show) {
      loader = (
                <ReactCSSTransitionGroup
                  transitionName="loader-animation"
                  transitionAppear={true}
                  component="div"
                  className={"line-scale-pulse-out-rapid"}>
                  <div style={style}></div>
                  <div style={style}></div>
                  <div style={style}></div>
                  <div style={style}></div>
                  <div style={style}></div>
                </ReactCSSTransitionGroup>
      );
    }else {
      loader = <span></span>
    }
    return loader;

  }
}

Loader.defaultProps = {
  show: true,
  size: '5'
}

