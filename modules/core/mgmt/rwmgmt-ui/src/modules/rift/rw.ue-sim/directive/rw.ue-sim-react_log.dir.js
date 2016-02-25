(function (window, angular, React, rw, undefined) {
  "use strict";

  /**
   * Takes logs sent to the react_log_service and renders the logs nessassary to fill the viewport.
   * */
  angular.module('reactLog', ['scriptLog'])

    .directive('reactLog', ['$interval', 'scriptLog', "ueScript", "$rootScope", "$timeout", function ($interval, scriptLog, ueScript, $rootScope, $timeout) {
      return {
        restrict: 'E',
        scope: {
          framework: '='
        },
        template: '<div class="controls">' +
        '<dl class="drpdwn-cntrl">' +
        '<dt>Verbosity</dt>' +
        '<dd id="verbosityLevel" ng-class="{\'drpdwn-grp\':true, open:verbosityLevelToggle}"  ng-init="verbosityLevelSelect = 0">' +
        '<a class="drpdwn-tgl" href="">{{verbosityLevelSelect}} <span class="icn fa-sort-desc"></span></a>' +
        '<ul class="drpdwn">' +
        '<li><a href="" ng-click="setVerbosity(0)">0</a></li>' +
        '<li><a href="" ng-click="setVerbosity(1)">1</a></li>' +
        '<li><a href="" ng-click="setVerbosity(2)">2</a></li>' +
        '<li><a href="" ng-click="setVerbosity(3)">3</a></li>' +
        '<li><a href="" ng-click="setVerbosity(4)">4</a></li>' +
        '<li><a href="" ng-click="setVerbosity(5)">5</a></li>' +
        '</ul>' +
        '</dd>' +
        '<dl class="drpdwn-cntrl">' +
        '<dt>Severity</dt>' +
        '<dd id="severityLevel" ng-class="{\'drpdwn-grp\':true, open:severityLevelToggle}"  ng-init="severityLevelSelect = \'debug\'">' +
        '<a class="drpdwn-tgl" href="">{{severityLevelSelect}} <span class="icn fa-sort-desc"></span></a>' +
        '<ul class="drpdwn">' +
        '<li><a href="" ng-click="setSeverity(\'debug\')">debug</a></li>' +
        '<li><a href="" ng-click="setSeverity(\'info\')">info</a></li>' +
        '<li><a href="" ng-click="setSeverity(\'notice\')">notice</a></li>' +
        '<li><a href="" ng-click="setSeverity(\'warning\')">warning</a></li>' +
        '<li><a href="" ng-click="setSeverity(\'error\')">error</a></li>' +
        '<li><a href="" ng-click="setSeverity(\'critical\')">critical</a></li>' +
        '<li><a href="" ng-click="setSeverity(\'alert\')">alert</a></li>' +
        '<li><a href="" ng-click="setSeverity(\'emergency\')">emergency</a></li>' +
        '</ul>' +
        '</dd>' +
        '</dl>' +
        '</div>' +
        '<div id="reactlog"></div>'+
        '<div class="controls">'+
        '<ul class="right">'+
        '<li><a href="" ng-click="clear()" class="button-link">Clear Log</a></li>'+
        '<li><a href="" ng-click="reloadLogs()" class="button-link">Reload</a></li>'+
        '<li><a href="" ng-click="downloadText()" class="button-link">Save to File</a></li>'+
        '</ul>'+
        '</div>',
        link: linkFn
      };

      /**
       * Takes care of the button and dropdown logic for
       **/
      function linkFn(scope, element, attrs) {
        var scriptLogInt;
        scope.stop;
        scope.records = [];
        scope.line_height = 0;
        var first = true;
        var renderOfflineInterval;




        /**
         * Clears the log and local cache of logs.
         */
        scope.clear = function(){
          scope.records = [];
          scriptLog.clearLogs();
          renderReact(element,scriptLog.showLogs());
        };

        /**
         * Event listener to close all dropdowns if the user clicks on the document
         */
        $(document).click(function() {
          scope.verbosityLevelToggle = false;
          scope.severityLevelToggle = false;
          scope.$apply()
        });

        /**
         * Verbosity dropdown logic.
         * @param num
         */
        scope.setVerbosity = function(num) {
          scope.verbosityLevelSelect = num;
          scriptLog.setVerbosity(num);
        };
        /**
         * Severity dropdown logic.
         * @param severity
         */
        scope.setSeverity = function(severity) {
          scope.severityLevelSelect = severity;
          scriptLog.setSeverity(severity);
        };

        /**
         * Verbosity dropdown click event listener.
         */
        $('#verbosityLevel').click(function(e) {
          scope.verbosityLevelToggle = !scope.verbosityLevelToggle;
          e.stopPropagation();
          scope.$apply();
        });

        /**
         * Severity dropdown click event listener.
         */
        $('#severityLevel').click(function(e) {
          scope.severityLevelToggle = !scope.severityLevelToggle;
          e.stopPropagation();
          scope.$apply();
        });


        /**
         * Clears the logs then asks the backend to resend all logs for the current script.
         */
        scope.reloadLogs = function() {
          scope.clear();  //Liam's function written up above.

          var script_id = scriptLog.getID();
          var script_obj = {id: script_id};
          if (script_id>= 0) {
            //console.log("DEBUG:: Reload running");
            //console.log("DEBUG:: " + scriptLog.getID());
            scriptLog.clearLogs();
            try{
              scriptLog.stopTimeout();
            } catch (e){
              console.log(e)
            }
            scriptLog.setStopLog(true);
            //console.log("DEBUG::");
            //console.log(script_obj);
            scriptLog.startLog(script_id, function(script_obj){
              scope.records = scriptLog.showLogs();
              renderReact(element,scope.records);
            });
          }
        };

        /**
         * When the script starts running, we clear previous logs, intervals, and script ids. Then we begin loading
         * the new logs into the cache.
         */
        $rootScope.$on('ue.script.running',
          function (event, data) {
            if(!rw.api.offline){
              scriptLog.clearLogs();

              try{
                scriptLog.stopTimeout();
              } catch (e){
                console.log(e)
              }
              scriptLog.setStopLog(true);
              scriptLog.setID(data.id);
              scriptLog.startLog(data.id, function(data){
                scope.records = scriptLog.showLogs();
                renderReact(element,scope.records);
              });


            }else{
              scriptLog.clearLogs();
              renderOfflineInterval = setInterval(function(){
                renderReact(element, data.logs)
              },1000)
              $rootScope.$on('ue.script.off', function(){
                clearInterval(renderOfflineInterval);
                scriptLog.stopLog();
              })
            }
          }
        );

        /**
         * Downloads a text file of the formatted logs onto the user's
         * computer.
         */
        scope.downloadText = function () {
          var blob = new Blob([scope.concatRecordsForSave()], {type: "text/plain;charset=utf-8"})
          saveAs(blob, "log.txt");
        };

        /**
         * Formats logs into a readable form for download.
         * @returns {string}
         */
        scope.concatRecordsForSave = function () {
          var ret = "";
          for (var i = 0; i < scope.records.length; i++) {
            for (var key in scope.records[i]) {
              if (scope.records[i].hasOwnProperty(key)) {
                ret += " " + key +  " :: " + scope.records[i][key] + "\r\n";
              }
            }
            ret += "\r\n\r\n";
          }
          return ret;
        };

        /**
         * Initializes line height to be passed down to the react log logic.
         * @param el
         * @param log
         */
        function renderReact(el, log) {
          var first_time = function () {};
          if (first) {
            first_time = function () {
              if ($('#reactlog .w2ui-even')[0] != null) {
                scope.line_height = $('#reactlog .w2ui-even')[0].clientHeight;
              } else {
                scope.line_height = 58;
              }
            };
            first = false;
          }


          /**
           * Renders the react log after initializing default variables.
           */
          React.render(
            React.createElement(Grid, {
                width: 600,
                height: 400,
                header: "List of Names",
                showToolbar: true,
                name: "grid",
                records: log,
                // In pixels
                lineHeight: scope.line_height,
                viewportPaddingBottom: 15
              }
            ),
            document.getElementById('reactlog'),
            first_time
          );
        }
      }


    }])



})(window, window.angular, window.React || window.$rw.component.React, window.rw);

if (typeof React == 'undefined') {
  var React = window.$rw.component.React;
}

/** @jsx React.DOM */

var GridBody = React.createClass({
  displayName: 'GridBody',
  /** Sets a default state if a state is not defined */
  getInitialState: function () {
    return {
      // make into function
      shouldUpdate: true,
      total: 0,
      displayStart: 0,
      displayEnd: 0,
      toBottom: false
    };
  },

  /** When props are getting updated, this function checks if react needs to re-render DOM Elements */
  componentWillReceiveProps: function (nextProps) {
    var shouldUpdate = !(
      nextProps.visibleStart >= this.state.displayStart &&
      nextProps.visibleEnd <= this.state.displayEnd
      ) || (nextProps.total !== this.state.total);

    if (shouldUpdate) {
      this.setState({
        shouldUpdate: shouldUpdate,
        total: nextProps.total,
        displayStart: nextProps.displayStart,
        displayEnd: nextProps.displayEnd
      });
    } else {
      this.setState({shouldUpdate: false});
    }
  },

  /** Re-renders the DOM if needed */
  shouldComponentUpdate: function (nextProps, nextState) {
    return this.state.shouldUpdate;
  },

  /** Renders the logs */
  render: function () {

    this.displayStart = 0;
    var rows = {};

    // Creates the top buffer for the logs.  Instead of rendering all logs above the viewport, we use this div to give
    // The impression the logs are being rendered.
    rows.top = (
      React.createElement("div", {
          style: {height: this.props.displayStart * this.props.recordHeight}
        },
        React.createElement("div", {})
      )
    );


    // This the logic that forces the viewport to display the most recent logs if the viewport is
    // within viewportPaddingBottom pixels of the bottom of the grid container.
    if (this.props.gridEle) {
      var gridNode = this.props.gridEle.getDOMNode();
      var totalHeight = gridNode.scrollHeight;
      var viewHeight = $(gridNode).height();
      var scrollLoc = gridNode.scrollTop;
      var bottom = scrollLoc - viewHeight;

      if (scrollLoc + viewHeight >= totalHeight - this.props.viewportPaddingBottom) {
        setTimeout((function (node) {
          totalHeight = node.scrollHeight;

          node.scrollTop = totalHeight;
        }), 200, gridNode);
      }
    } /*else {
     setTimeout(function (node) {
     totalHeight = node.scrollHeight;
     node.scrollTop = totalHeight;

     }, 200, gridNode);
     }*/

    // Extracts the logs from records and creates a div for each one.
    if (this.props.records.length > 0) {
      for (var i = this.props.displayStart; i <= this.props.displayEnd; ++i) {
        var message = "";
        var messageTotal = [];
        var j = 0;
        // COMMENT THIS
        var record = this.props.records[i];
        if (!record) {
          record = {msg: ' '};  //This is so we can iterate over nothing
        }

        // Each record object holds several properties, each must be displayed.
        // Look into underscore library. <--
        for (var key in this.props.records[i]) {
          if (this.props.records[i].hasOwnProperty(key)) {
            messageTotal[j] = " " + key + " " + this.props.records[i][key];
            j++;
          }
        }
        // \r\r\n\n creates new lines in IE
        message = messageTotal.join("\n\n\r\r");
        rows['lineid_' + i] = (
          React.createElement("div", {}, message)
        );
      }
    }

    // Creates the bottom buffer for the logs.  Instead of rendering all logs below the viewport, we use this div to give
    // The impression the logs are being rendered.
    rows.bottom = (
      React.createElement("div", {id: "gridgridrecbottom", line: "bottom",
          style: {height: ((this.props.records.length - 1) - this.props.displayEnd) * this.props.recordHeight}}
      )
    );

    return (
      React.createElement("div", null,
        rows
      )
    );
  }
});

/** Creates the React container for the logs. */
var Grid = React.createClass({
  displayName: 'Grid',

  /** Creates the default state from the variables passed in. */
  getDefaultState: function (props) {
    var recordHeight = props.lineHeight;
    var recordsPerBody = Math.min(Math.max(Math.floor(($('#grid').height() - 2) / recordHeight), 0), Math.floor(props.records.length / 2));
    console.log(recordsPerBody);
    return {
      total: props.records.length,
      records: props.records,
      viewportPaddingBottom: props.viewportPaddingBottom,
      recordHeight: recordHeight,
      recordsPerBody: recordsPerBody,
      // This is what decides the viewport measurements. The program starts showing DOM elements starting at the display start and stops at the display end.
      // The visible start and end is where the presumed viewport begins and ends.
      visibleStart: 0,
      visibleEnd: recordsPerBody,
      displayStart: 0,
      displayEnd: recordsPerBody * 2,
      viewportHeight: 20, // In logs
      viewportBuffer: 50 // Logs displayed above and below the viewport.
    };
  },

  /** On recieving new logs, re-calculate the viewport location */
  componentWillReceiveProps: function (nextProps) {
    this.setState(this.getDefaultState(nextProps));
    this.scrollState(this.refs.scrollable.getDOMNode().scrollTop);
  },

  /** Initial state is the same as default state */
  getInitialState: function () {
    return this.getDefaultState(this.props);
  },

  /**
   *  On Scroll this is called.  This defines viewport variables
   *  visibleStart and visibleEnd define the start and end points of the viewport
   *  displayStart and displayEnd define the start and end points of which logs we render.
   *  The unit is not pixels but index of logs.
   * */
  scrollState: function (scroll) {
    scroll = Math.floor(scroll/this.state.recordHeight);
    var visibleStart = Math.max(scroll, 0);
    var visibleEnd = Math.min(visibleStart + this.state.viewportHeight, Math.max(this.props.records.length - 1, 0));
    var displayStart = Math.max(0, visibleStart - this.state.viewportBuffer);
    var displayEnd = Math.min(this.props.records.length - 1, visibleEnd);

    this.setState({
      visibleStart: visibleStart,
      visibleEnd: visibleEnd,
      displayStart: displayStart,
      displayEnd: displayEnd,
      scroll: scroll
    });
  },

  /** Creates an event listener for scrolling on the grid container */
  onScroll: function (event) {
    this.scrollState(this.refs.scrollable.getDOMNode().scrollTop);
  },

  /** Renders the grid container */
  render: function () {
    var self = this;
    return (
      React.createElement("div", {
          id: "grid",
          ref:"scrollable",
          // Initialize these values.
          style: {width: "100%", height: 400, "overflow-y":'auto'},
          name: "grid",
          onScroll: self.onScroll
        },


        React.createElement(GridBody, {
            // Pass down state rather than bunches of the state variable.
            records: this.state.records,
            total: this.state.records.length,
            visibleStart: this.state.visibleStart,
            visibleEnd: this.state.visibleEnd,
            displayStart: this.state.displayStart,
            displayEnd: this.state.displayEnd,
            recordHeight: this.state.recordHeight,
            viewportPaddingBottom: this.state.viewportPaddingBottom,
            // grid Element
            gridEle: this.refs.scrollable
          }
        )
      )
    );
  }
});






