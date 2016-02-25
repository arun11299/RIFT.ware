'use strict';

var _extends = Object.assign || function (target) { for (var i = 1; i < arguments.length; i++) { var source = arguments[i]; for (var key in source) { if (Object.prototype.hasOwnProperty.call(source, key)) { target[key] = source[key]; } } } return target; };

function _interopRequireDefault(obj) { return obj && obj.__esModule ? obj : { 'default': obj }; }

var _react = require('react');

var _react2 = _interopRequireDefault(_react);

var _Tab = require('./Tab');

var _Tab2 = _interopRequireDefault(_Tab);

module.exports = _react2['default'].createClass({
  displayName: 'MoreTab',

  propTypes: {
    maxCount: _react2['default'].PropTypes.number,
    moreCount: _react2['default'].PropTypes.number
  },

  getDefaultProps: function getDefaultProps() {
    return {
      maxCount: 0
    };
  },

  render: function render() {
    return _react2['default'].createElement(
      _Tab2['default'],
      _extends({ visible: this.props.moreCount > 0
      }, this.props),
      _react2['default'].createElement(
        'div',
        null,
        this.props.moreCount,
        ' More ▼'
      )
    );
  }
});