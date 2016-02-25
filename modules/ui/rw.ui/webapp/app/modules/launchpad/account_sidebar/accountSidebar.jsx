/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
import React from 'react';
import './accountSidebar.scss';
import DashboardCard from '../../components/dashboard_card/dashboard_card.jsx';
import CloudAccountStore from '../launchpad_cloud_account/cloudAccountStore.js';
import ConfigAgentAccountStore from '../launchpad_config_agent_account/configAgentAccountStore';
import SdnAccountStore from '../../missioncontrol/sdn_account/createSdnAccountStore';

export default class AccountSidebar extends React.Component{
    constructor(props) {
        super(props);
        var self = this;
        this.state = {}
        this.state.cloudAccounts = CloudAccountStore.getState().cloudAccounts || [];
        CloudAccountStore.listen(function(state) {
            self.setState({
                cloudAccounts: state.cloudAccounts
            });
        });
        CloudAccountStore.getCloudAccounts();

        this.state.configAgentAccounts = ConfigAgentAccountStore.getState().configAgentAccounts || [];
        ConfigAgentAccountStore.listen(function(state) {
            self.setState({
                configAgentAccounts: state.configAgentAccounts
            });
        });
        ConfigAgentAccountStore.getConfigAgentAccounts();

        this.state.sdnAccounts = SdnAccountStore.getState().sdnAccounts || [];
        SdnAccountStore.listen(function(state) {
            self.setState({
                sdnAccounts: state.sdnAccounts
            });
        });
        SdnAccountStore.getSdnAccounts();
    }
    createCloudAccount = () => {
        window.location.hash = "#/launchpad/" + window.location.hash.split('/')[2] + "/cloud-account/create";
    }
    createSDNAccount = () => {
        window.location.hash = "#/launchpad/" + window.location.hash.split('/')[2] + "/sdn-account/create";
    }
    createConfigAgentAccount = () => {
        window.location.hash = '#/launchpad/' + window.location.hash.split('/')[2] + '/config-agent-account/create';
    }
    render() {
        let html;
        let cloudAccounts = (this.state.cloudAccounts.length > 0) ? this.state.cloudAccounts.map(function(account, index) {
            return (
                <DashboardCard className='pool-card accountSidebarCard'>
                <header>
                    <a href={"#/launchpad/" + window.location.hash.split('/')[2] + "/cloud-account/" + account.name + '/edit' }>
                        <h3>{account.name}</h3>
                    </a>
                </header>
                    {
                        account.pools.map(function(pool, i) {
                        // return <a className={pool.type + ' link-item'} key={i} href={'#/pool/' + account.name +'/' + pool.name}>{pool.name}</a>
                        })
                    }
                    <a className="empty-pool link-item"  href={"#/pool/" + account.name + '/' } style={{cursor: 'pointer', display: 'none'}}>
                        <span>
                            <h2 className="create-title">Create Pool</h2>
                        </span>
                        <img src={require("../../../assets/img/launchpad-add-fleet-icon.png")}/>
                    </a>
                </DashboardCard>
            )
        }) : null;
        let sdnAccounts = (this.state.sdnAccounts && this.state.sdnAccounts.length > 0) ? this.state.sdnAccounts.map(function(account, index) {
            return (
                <DashboardCard  className='pool-card accountSidebarCard'>
                     <header>
                        <a href={"#/launchpad/" + window.location.hash.split('/')[2] + "/sdn-account/" + account.name + '/edit' }>
                            <h3>{account.name}</h3>
                        </a>
                    </header>
                </DashboardCard>
            )
        }) : null;
        let configAgentAccounts = (this.state.configAgentAccounts.length > 0) ? this.state.configAgentAccounts.map(function(account, index) {
            return (
                <DashboardCard className='pool-card accountSidebarCard'>
                <header>
                    <a href={"#/launchpad/" + window.location.hash.split('/')[2] + "/config-agent-account/" + account.name + '/edit' }>
                        <h3>{account.name}</h3>
                    </a>
                </header>
                </DashboardCard>
            )
        }) : null;
        html = (
            <div className='accountSidebar'>
                <h1>Cloud Accounts</h1>
                {cloudAccounts}
                <DashboardCard className="accountSidebarCard">
                        <div onClick={this.createCloudAccount} className={'accountSidebarCard_create'} style={{cursor:'pointer'}}>
                            Add Cloud Account
                            <img src={require("../../../assets/img/launchpad-add-fleet-icon.png")}/>
                        </div>
                </DashboardCard>
                <h1>SDN Accounts</h1>
                {sdnAccounts}
                <DashboardCard className="accountSidebarCard">
                        <div onClick={this.createSDNAccount}  className={'accountSidebarCard_create'} style={{cursor:'pointer'}}>
                            Add SDN Account
                            <img src={require("../../../assets/img/launchpad-add-fleet-icon.png")}/>
                        </div>
                </DashboardCard>
                <h1>Config Agent Accounts</h1>
                {configAgentAccounts}
                <DashboardCard className="accountSidebarCard">
                        <div onClick={this.createConfigAgentAccount} className={'accountSidebarCard_create'} style={{cursor:'pointer'}}>
                            Add Config Agent Account
                            <img src={require("../../../assets/img/launchpad-add-fleet-icon.png")}/>
                        </div>
                </DashboardCard>
            </div>
                );
        return html;
    }
}

AccountSidebar.defaultProps = {
    cloudAccounts: [],
    sdnAccounts: [],
    configAgentAccounts: []
}
