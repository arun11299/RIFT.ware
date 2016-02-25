import React from 'react';
import './accountSidebar.scss';
import DashboardCard from '../../components/dashboard_card/dashboard_card.jsx';

export default class AccountSidebar extends React.Component{
    constructor(props) {
        super(props);
    }
    createCloudAccount = () => {
        window.location.hash = "#/cloud-account/create";
    }
    render() {
        let html;
        let cloudAccounts = this.props.cloudAccounts.map(function(account, index) {
            return (
                <DashboardCard className='pool-card accountSidebarCard'>
                <header>
                    <a href={'#/cloud-account/' + account.name +'/edit'}>
                        <h3>{account.name}</h3>
                    </a>
                </header>
                    {
                        account.pools.map(function(pool, i) {
                        return <a className={pool.type + ' link-item'} key={i} href={'#/pool/' + account.name +'/' + pool.type + '/' + pool.name}>{pool.name}</a>
                        })
                    }
                    <a className="empty-pool link-item"  href={"#/pool/" + account.name + '/create' } style={{cursor:'pointer'}}>
                        <span>
                            <h2 className="create-title">Create Pool</h2>
                        </span>
                        <img src={require("../../../assets/img/launchpad-add-fleet-icon.png")}/>
                    </a>
                </DashboardCard>
            )
        });
        let sdnAccounts = (this.props.sdnAccounts.length > 0) ? this.props.sdnAccounts.map(function(account, index) {
            return (
                <DashboardCard title={account.name} showHeader={true} className='pool-card accountSidebarCard'>
                </DashboardCard>
            )
        }) : [];
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
                        <div className={'accountSidebarCard_create'} style={{cursor:'pointer'}}>
                            Add SDN Account
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
    sdnAccounts: []
}
