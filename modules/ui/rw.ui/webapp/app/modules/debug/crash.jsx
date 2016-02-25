
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
import React from 'react/addons';
import './crash.scss';
import TreeView from 'react-treeview';
import '../../../node_modules/react-treeview/react-treeview.css';
import AppHeader from '../components/header/header.jsx';
var crashActions = require('./crashActions.js');
var crashStore = require('./crashStore.js');
var MissionControlStore = require('../missioncontrol/missionControlStore.js');
function openDashboard() {
  window.location.hash = "#/";
}
class CrashDetails extends React.Component {
  constructor(props) {
    super(props)
    var self = this;
    crashStore.listen(function(data) {
      self.setState({
        list:data.crashList,
        noDebug:!self.hasDebugData(data.crashList)
      })
    });
    crashStore.get();
  }
  hasDebugData(list) {
    console.log(list);
    if (list && list.length > 0) {
      for (let i = 0; i < list.length; i++) {
        var trace = list[i].backtrace;
        for (let j = 0; j < trace.length; j++) {
          console.log(trace[j])
          if (trace[j].detail) {
            return true;
          }
        }
      }
    }
    return false;
  }
  downloadFile(fileName, urlData) {
    var aLink = document.createElement('a');
    var evt = document.createEvent("HTMLEvents");
    evt.initEvent("click");
    aLink.download = fileName;
    aLink.href = urlData;
    aLink.dispatchEvent(evt);
  }
  openAbout = function() {
    window.location.hash = "#/about"
  }
  openLog = function() {
    MissionControlStore.getSysLogViewerURL('mc');

  }

  render() {
    let html;
    var list = null;
    if (this.state != null) {
      var tree = <div style={{'margin-left':'auto', 'margin-right':'auto', 'width':'230px', 'padding':'90px'}}> No Debug Information Available </div>;
      if (!this.state.noDebug)
      {
        var tree = this.state.list.map((node, i) => {
                  var vm = node.name;
                  var vm_label = <span>{vm}</span>;
                  var backtrace = node.backtrace;
                  return (
                    <TreeView key={vm + '|' + i} nodeLabel={vm_label} defaultCollapsed={false}>
                      {backtrace.map(details => {

                        //Needed to decode HTML
                        var text_temp = document.createElement("textarea")
                        text_temp.innerHTML = details.detail;
                        var text_temp = text_temp.value;
                        var arr = text_temp.split(/\n/);
                        var text = [];
                        for (let i = 0; i < arr.length; i++) {
                          text.push(arr[i]);
                          text.push(<br/>)
                        }

                        return (
                          <TreeView nodeLabel={<span>{details.id}</span>} key={vm + '||' + details.id} defaultCollapsed={false}>
                            <p>{text}</p>
                          </TreeView>
                        );
                      })}
                    </TreeView>
                  );
                })}
      html = (
        <div>
              <div className="form-actions">
                <a role="button" className="primary" onClick={this.state.noDebug ? false : this.downloadFile.bind(this, 'crash.txt', 'data:text;charset=UTF-8,' + encodeURIComponent(JSON.stringify(this.state.list)))}> Download Crash Details</a>
              </div>
              <div className="crash-container">
                <h2> Debug Information </h2>
                <div className="tree">{tree}</div>
              </div>
        </div>
              );
    } else {
      html = <div className="crash-container"></div>
    };
    let refPage = window.sessionStorage.getItem('refPage');
    refPage = JSON.parse(refPage);
    let navItems = [{
      name: refPage.title,
      onClick: function() {
        window.location.hash = refPage.hash
      }
      }];
    return (
            <div className="crash-app">
              <AppHeader title={'Launchpad: Debug'} nav={navItems} />{html}
            </div>
            );
  }
}
export default CrashDetails;
