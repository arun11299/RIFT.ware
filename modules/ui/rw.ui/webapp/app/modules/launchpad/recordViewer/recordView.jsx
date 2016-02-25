
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
import React from 'react';
import RecordNavigator from './recordNavigator.jsx';
import RecordCard from './recordCard.jsx';
import RecordDetails from './recordDetails.jsx';
import RecordViewStore from './recordViewStore.js';
import RecordViewActions from './recordViewActions.js';
import LaunchpadBreadcrumbs from '../launchpadBreadcrumbs.jsx';
import Utils from '../../utils/utils.js';
import AppHeader from '../../components/header/header.jsx';
import './recordViewer.scss';
export default class RecordView extends React.Component {
  constructor(props) {
    super(props);
    this.state = RecordViewStore.getState();
    this.state.showRecordDetails = false;
    RecordViewStore.listen(this.storeListener);
  }
  openLog() {
      var LaunchpadStore = require('../launchpadFleetStore.js')
      LaunchpadStore.getSysLogViewerURL('lp');
  }
  openAbout = () => {
    this.componentWillUnmount();
    let loc = window.location.hash.split('/');
    loc.pop();
    loc.pop();
    loc.push('lp-about');
    window.location.hash = loc.join('/');
  }
  openDebug = () =>  {
    this.componentWillUnmount();
    let loc = window.location.hash.split('/');
    loc.pop();
    loc.pop();
    loc.push('lp-debug');
    window.location.hash = loc.join('/');
  }
  storeListener = (state) => {
    this.setState(state);
  }
  componentWillUnmount = () => {
    this.state.socket.close();
    RecordViewStore.unlisten(this.storeListener);
  }
  componentDidMount() {
    let nsrRegEx = new RegExp("([0-9a-zA-Z-]+)\/detail");
    let nsr_id;
    try {
      console.log('NSR ID in url is', window.location.hash.match(nsrRegEx)[1]);
      nsr_id = window.location.hash.match(nsrRegEx)[1];
    } catch (e) {

    }
    RecordViewStore.getNSR(nsr_id);
    RecordViewStore.getRawNSR(nsr_id);
    RecordViewStore.getNSRSocket(nsr_id);
  }
  loadRecord = (record) => {
    RecordViewActions.loadRecord(record);
    RecordViewStore['getRaw' + record.type.toUpperCase()](record.id)
    RecordViewStore['get' + record.type.toUpperCase() + 'Socket'](record.id)
  }
  recordDetailsToggle = () => {
    this.setState({
      showRecordDetails: !this.state.showRecordDetails
    })
  }
  render() {
    let html;
    let mgmtDomainName = window.location.hash.split('/')[2];
    let nsrId = window.location.hash.split('/')[3];
    let recordDetails = this.state.showRecordDetails || null;
    let navItems = [{
      href: '#/launchpad/' + mgmtDomainName,
      name: 'DASHBOARD',
      onClick: this.componentWillUnmount
    },{
      name: 'Viewport'
    },
    {
      href: '#/launchpad/' + mgmtDomainName + '/' + nsrId + '/topology',
      name: 'TOPOLOGY',
      onClick: this.componentWillUnmount
    }
    // Commented out for OSM_MWC
    // ,{
    //     href: '#/launchpad/' + mgmtDomainName + '/' + nsrId + '/topologyL2',
    //     name: 'TOPOLOGYL2',
    //     onClick: this.componentWillUnmount
    // }, {
    //     href: '#/launchpad/' + mgmtDomainName + '/' + nsrId + '/topologyL2Vm',
    //     name: 'TOPOLOGYL2VM',
    //     onClick: this.componentWillUnmount
    // }
    ];
    let nav = <AppHeader title="Launchpad: Viewport" nav={navItems} />
    if (this.state.showRecordDetails) {
      recordDetails = <RecordDetails isLoading={this.state.detailLoading} data={this.state.rawData} />
    }
    html = (
      <div className="app-body">
        {nav}
        <div className="recordViewer">
          <i className="corner-accent top left"></i>
          <i className="corner-accent top right"></i>
          <div className="dashboardCard_wrapper recordPanels">
            <RecordNavigator activeNavID={this.state.recordID} nav={this.state.nav} loadRecord={this.loadRecord} isLoading={this.state.isLoading} />
            <RecordCard isLoading={this.state.cardLoading} type={this.state.recordType} data={this.state.recordData} recordDetailsToggleValue={this.state.showRecordDetails} recordDetailsToggleFn={this.recordDetailsToggle} />
            {recordDetails}
          </div>
          <i className="corner-accent bottom left"></i>
          <i className="corner-accent bottom right"></i>
        </div>
      </div>
    );
    return html;
  }
}
