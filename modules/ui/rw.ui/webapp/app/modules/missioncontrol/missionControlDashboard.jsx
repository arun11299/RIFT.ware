
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
import React from 'react/addons';
import ManagementDomainCard from './management_domain/management_domain_card/managementDomainCard.jsx';
import MissionControlStore from './missionControlStore.js';
import MissionControlActions from './missionControlActions.js'
import TransmitReceive from '../components/transmit-receive/transmit-receive.jsx';
import AccountSidebar from './account_sidebar/accountSidebar.jsx';
import AppHeader from '../components/header/header.jsx';
import './missionControlDashboard.scss';
class MissionControlDashboard extends React.Component {
  constructor(props) {
    super(props);
    this.state = {
      data: this.props.data,
      managementDomains: []
    }
  }
  componentDidMount() {
    MissionControlStore.listen(this.handleUpdate.bind(this));
    MissionControlActions.validateReset();
  }
  componentWillUnmount() {
    MissionControlStore.closeSocket();
  }
  handleUpdate(data){
    this.setState({
      managementDomains: data.domains,
      cloudAccounts: data.cloudAccounts,
      sdnAccounts: data.sdnAccounts
    })
  }
  render() {
    let html;
    let managementDomains = [];
    let navItems = [{
      name: 'DASHBOARD',
      }];
    let nav = <AppHeader title={'MISSION CONTROL'} nav={navItems} />
    try{
    this.state.managementDomains.map(function(md, index) {
      let cardhtml = <ManagementDomainCard key={index} className="ManagementDomainCard" data={md} isActive={md.launchpad.state == "started"} status={md.launchpad.state}/>
      managementDomains.push(cardhtml);
    });
  } catch(e) {
    console.log(e)
  }
    html = (
            <div>
              {nav}
              <div className={'flex'}>
                <AccountSidebar cloudAccounts={this.state.cloudAccounts} sdnAccounts={this.state.sdnAccounts}/>
                <div className="management-domains">
                  <h1>Management Domains</h1>
                  <div className="dashboardCard_wrapper">
                    {managementDomains}
                    <ManagementDomainCard create={true}/>
                  </div>
                </div>
              </div>
            </div>
            )
    return html;
  }
}

MissionControlDashboard.defaultProps = {
  data: []
}

export default MissionControlDashboard;
