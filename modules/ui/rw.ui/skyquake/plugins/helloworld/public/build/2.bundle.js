webpackJsonp([2,1],{

/***/ 402:
/***/ function(module, exports, __webpack_require__) {

	var map = {
		"./skyquakeAltInstance": 365,
		"./skyquakeAltInstance.js": 365,
		"./skyquakeApp": 403,
		"./skyquakeApp.js": 403,
		"./skyquakeContainer": 353,
		"./skyquakeContainer.jsx": 353,
		"./skyquakeContainerActions": 384,
		"./skyquakeContainerActions.js": 384,
		"./skyquakeContainerSource": 381,
		"./skyquakeContainerSource.js": 381,
		"./skyquakeContainerStore": 380,
		"./skyquakeContainerStore.js": 380,
		"./skyquakeNav": 379,
		"./skyquakeNav.jsx": 379,
		"./skyquakeRouter": 404,
		"./skyquakeRouter.js": 404
	};
	function webpackContext(req) {
		return __webpack_require__(webpackContextResolve(req));
	};
	function webpackContextResolve(req) {
		return map[req] || (function() { throw new Error("Cannot find module '" + req + "'.") }());
	};
	webpackContext.keys = function webpackContextKeys() {
		return Object.keys(map);
	};
	webpackContext.resolve = webpackContextResolve;
	module.exports = webpackContext;
	webpackContext.id = 402;


/***/ },

/***/ 403:
/***/ function(module, exports) {

	"use strict";

/***/ },

/***/ 404:
/***/ function(module, exports, __webpack_require__) {

	'use strict';
	
	var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();
	
	var _typeof = typeof Symbol === "function" && typeof Symbol.iterator === "symbol" ? function (obj) { return typeof obj; } : function (obj) { return obj && typeof Symbol === "function" && obj.constructor === Symbol ? "symbol" : typeof obj; };
	
	Object.defineProperty(exports, "__esModule", {
	    value: true
	});
	
	exports.default = function (config, context) {
	    var routes = [];
	    var index = null;
	    var components = null;
	    if (config && config.routes) {
	        routes = config.routes.map(function (route, index) {
	            if (route.route && route.component) {
	                var _ret = function () {
	                    // ES6 modules need to specify default
	                    var component = context(route.component).default;
	                    var path = route.route;
	                    // return (<Route key={index} path={route.route} component={component} />);
	                    return {
	                        v: {
	                            getComponent: function getComponent(location, cb) {
	                                !/* require.ensure */(function (require) {
	                                    console.log(component);
	                                    cb(null, __webpack_require__(402)(component));
	                                }(__webpack_require__));
	                            },
	                            path: path
	                        }
	                    };
	                }();
	
	                if ((typeof _ret === 'undefined' ? 'undefined' : _typeof(_ret)) === "object") return _ret.v;
	            } else {
	                console.error('Route not properly configured. Check that both path and component are specified');
	            }
	        });
	        if (config.dashboard) {
	            // ES6 modules need to specify default
	            index = _react2.default.createElement(_reactRouter.IndexRoute, { component: context(config.dashboard).default });
	        }
	
	        var rootRoute = {
	            component: _skyquakeContainer2.default,
	            childRoutes: [{
	                path: '/',
	                component: config.dashboard ? context(config.dashboard).default : DefaultDashboard,
	                childRoutes: routes
	            }]
	        };
	
	        return _react2.default.createElement(_reactRouter.Router, { history: _reactRouter.browserHistory, routes: rootRoute });
	    } else {
	        console.error('There are no routes configured in the config.json file');
	    }
	};
	
	var _react = __webpack_require__(148);
	
	var _react2 = _interopRequireDefault(_react);
	
	var _reactRouter = __webpack_require__(304);
	
	var _skyquakeContainer = __webpack_require__(353);
	
	var _skyquakeContainer2 = _interopRequireDefault(_skyquakeContainer);
	
	function _interopRequireDefault(obj) { return obj && obj.__esModule ? obj : { default: obj }; }
	
	function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }
	
	function _possibleConstructorReturn(self, call) { if (!self) { throw new ReferenceError("this hasn't been initialised - super() hasn't been called"); } return call && (typeof call === "object" || typeof call === "function") ? call : self; }
	
	function _inherits(subClass, superClass) { if (typeof superClass !== "function" && superClass !== null) { throw new TypeError("Super expression must either be null or a function, not " + typeof superClass); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, enumerable: false, writable: true, configurable: true } }); if (superClass) Object.setPrototypeOf ? Object.setPrototypeOf(subClass, superClass) : subClass.__proto__ = superClass; }
	
	var DefaultDashboard = function (_React$Component) {
	    _inherits(DefaultDashboard, _React$Component);
	
	    function DefaultDashboard(props) {
	        _classCallCheck(this, DefaultDashboard);
	
	        return _possibleConstructorReturn(this, Object.getPrototypeOf(DefaultDashboard).call(this, props));
	    }
	
	    _createClass(DefaultDashboard, [{
	        key: 'render',
	        value: function render() {
	            var html = undefined;
	            html = _react2.default.createElement(
	                'div',
	                null,
	                ' This is a default dashboard page component '
	            );
	            return html;
	        }
	    }]);
	
	    return DefaultDashboard;
	}(_react2.default.Component);

/***/ }

});
//# sourceMappingURL=2.bundle.js.map