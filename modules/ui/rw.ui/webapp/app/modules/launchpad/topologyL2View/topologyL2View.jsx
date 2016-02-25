
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
import React from 'react';
import TopologyL2Store from './topologyL2Store.js';
import RecordDetail from '../recordViewer/recordDetails.jsx';
import './topologyL2View.scss';
import DashboardCard from '../../components/dashboard_card/dashboard_card.jsx';
import AppHeader from '../../components/header/header.jsx';
import TopologyL2Graph from '../../components/topology/topologyL2Graph.jsx';

export default class TopologyL2view extends React.Component {
    constructor(props) {
        super(props);
        //console.log("\n=====================================================");
        //console.log("TopologyL2view constructor called. props=", props);
        this.state = TopologyL2Store.getState();
        TopologyL2Store.listen(this.storeListener);
    }
    openLog() {
        var LaunchpadStore = require('../launchpadFleetStore.js')
        LaunchpadStore.getSysLogViewerURL('lp');
    }
    openAbout() {
        let loc = window.location.hash.split('/');
        loc.pop();
        loc.pop();
        loc.push('lp-about');
        window.location.hash = loc.join('/');
    }
    openDebug() {
        let loc = window.location.hash.split('/');
        loc.pop();
        loc.pop();
        loc.push('lp-debug');
        window.location.hash = loc.join('/');
    }
    storeListener = (state) => {
        this.setState(state);
    }

    componentWillUnmount() {
        TopologyL2Store.unlisten(this.storeListener);
    }
    componentDidMount() {
        if (this.props.topologyType == 'vm') {
            TopologyL2Store.fetchVmTop();
        } else {
            TopologyL2Store.fetchStackedTop();
        }

    }

    topoTag(topologyType) {
        var info = {
            single: 'topologyL2',
            vm: 'topologyL2Vm'
        }
        return info[topologyType];
    }

    // TODO: handle when no data in this.state.data
    // TODO: Add RecordDetail. See original topology render function
    render() {
        let html;
        let mgmtDomainName = window.location.hash.split('/')[2];
        let nsrId = window.location.hash.split('/')[3];
        let navItems = [{
          href: '#/launchpad/' + mgmtDomainName,
          name: 'DASHBOARD',
          onClick: this.componentWillUnmount
        },
        {
          href: '#/launchpad/' + mgmtDomainName + '/' + nsrId + '/detail',
          name: 'VIEWPORT',
          onClick: this.componentWillUnmount
        },{
            href: '#/launchpad/' + mgmtDomainName + '/' + nsrId + '/topology',
            name: 'TOPOLOGY',
            onClick: this.componentWillUnmount
        }];

        switch(this.props.topologyType) {
            case 'single':
                navItems = navItems.concat([{
                    name: 'TOPOLOGYL2'
                }, {
                    href: '#/launchpad/' + mgmtDomainName + '/' + nsrId + '/topologyL2Vm',
                    name: 'TOPOLOGYL2VM',
                    onClick: this.componentWillUnmount
                }]);
                break;

            case 'vm':
                navItems = navItems.concat([{
                    href: '#/launchpad/' + mgmtDomainName + '/' + nsrId + '/topologyL2',
                    name: 'TOPOLOGYL2',
                    onClick: this.componentWillUnmount
                }, {
                    name: 'TOPOLOGYL2VM'
                }]);
                break;
        }

        let nav = <AppHeader title="Topology" nav={navItems} />

        html = (
            <div className="app-body">
                {nav}
                <div className="topologyL2View">
                    <i className="corner-accent top left"></i>
                    <i className="corner-accent top right"></i>
                    <TopologyL2Graph data={this.state.topologyData}  />
                    <i className="corner-accent bottom left"></i>
                    <i className="corner-accent bottom right"></i>
                </div>
            </div>
        );

        return html;
    }

}

