
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
import React from 'react/addons';
import DashboardCard from '../../../components/dashboard_card/dashboard_card.jsx';
import ManagementDomainCardHeader from './managementDomainCardHeader.jsx';
import './managementDomainCard.scss'
function openLaunch() {
  window.location.hash = window.location.hash + '/launch';
}

class ManagementDomainCard extends React.Component {
  constructor(props) {
    super(props);
  }
  componentWillReceiveProps(nextProps) {
  }
  shouldComponentUpdate(nextProps) {
    return true;
  }
  openCreate() {
    window.location.hash = window.location.hash + 'management-domain/create'
  }
  render() {
    let html;
     if(this.props.create){
      html = <DashboardCard className="managementDomainCard"><div className={'managementDomainCard_create'} onClick={this.openCreate} style={{cursor:'pointer'}}><img src={require("../../../../assets/img/launchpad-add-fleet-icon.png")}/> Create Management Domain </div> </DashboardCard>;
    } else {
      var pools_html = null;
      if (this.props.data.pools) {
        pools_html = (
          <dl>
            <dt>VM Pool: </dt>
            <dd>{this.props.data.pools.vm || null}</dd>
            <dt>Network Pool: </dt>
            <dd>{this.props.data.pools.network || null}</dd>
            <dt>IP Address: </dt>
            <dd>{this.props.data.launchpad.ip_address}</dd>
          </dl>
        )
      } else {
        pools_html = (
          <dl>
            <dt>VM Pool: </dt>
            <dd>None</dd>
            <dt>Network Pool: </dt>
            <dd>None</dd>
            <dt>IP Address: </dt>
            <dd>{this.props.data.pools ? this.props.data.launchpad.ip_address : null}</dd>
          </dl>
        )
      }
    html = (
      <DashboardCard className="managementDomainCard">
        <ManagementDomainCardHeader name={this.props.data.name} isActive={this.props.isActive} style="93px" data={this.props.data}/>
        <div className="content">
          {pools_html}
        </div>
      </DashboardCard>
    );
}
    // <dt>VM Pools</dt>
    //         <dd>{this.props.data.pools.vm}</dd>
    //         <dt>Network Pools</dt>
    //         <dd>{this.props.data.pools.network}</dd>
    return html;
  }
}
ManagementDomainCard.propTypes = {
  nsr: React.PropTypes.object,
  isActive: React.PropTypes.bool,
  name: React.PropTypes.string
 };
ManagementDomainCard.defaultProps = {
  name: 'Loading...',
  data: {},
  isActive: false
};
export default ManagementDomainCard
