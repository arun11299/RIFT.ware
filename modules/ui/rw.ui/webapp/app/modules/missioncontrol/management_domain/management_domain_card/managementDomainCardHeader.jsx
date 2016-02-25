
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
import React from 'react';
import UpTime from '../../../components/uptime/uptime.jsx';
import LaunchpadOperationalStatus from '../../../components/operational-status/launchpadOperationalStatus.jsx';
import MissionControlStore from '../../missionControlStore.js';
import ManagementDomainStore from '../managementDomainStore.js';
class ManagementDomainCardHeader extends React.Component {
  constructor(props) {
    super(props);
    this.state = {};
    this.state.displayStatus = false;
    this.state.isLoading = true;
    this.openDashboard = this.openDashboard.bind(this);
    this.openConsole = this.openConsole.bind(this);
    this.setStatus = this.setStatus.bind(this);
    this.toggleStatus = this.toggleStatus.bind(this);
  }

  componentWillReceiveProps(nextProps) {
    let lpState = nextProps.data.launchpad.state;
    if(lpState == 'stopped' || lpState == 'started') {
      this.setState({
        isLoading: false
      })
    } else {
      this.setState({
        isLoading: true
      })
    }
  }
  deleteLaunchpad(name) {
    if (confirm("Do you really want to delete management domain '" + name + "'?")) {
            ManagementDomainStore.delete(name);
    }
  }
  doneLoading() {
    console.log('Done loading')
    this.setState({
      isLoading: false
    });
  }
  //TODO instead of calling the store method, an action should be emitted and the store should respond.

  openDashboard() {
    if(this.props.isActive) {
          window.open('http://' + this.props.data.launchpad.ip_address + ':8000/index.html?api_server=http://localhost#/launchpad/' + this.props.data.name);
        }
  }
  openConsole() {
    console.log('open console clicked');
  }
  setStatus(name) {
    console.log('setting status for ', name)
    this.props.isActive ? MissionControlStore.stopLaunchpad(name) : MissionControlStore.startLaunchpad(name)
  }
  editMgmtDomain(mgmtDomainName) {
    window.location.href = '#/management-domain/' + mgmtDomainName + '/edit'
  }
  toggleStatus() {
  }
  render() {
    let self = this;
    let isLoading = this.state.isLoading;
    let isActive = this.props.isActive;
    let opStatusHack = [];
    let d = new Date();
    let currentTime = (d.getTime() / 1000);
    let startTime = currentTime - this.props.data.launchpad["create-time"];
    opStatusHack.push({
      id:this.props.data.launchpad.state,
      description:this.props.data.launchpad.state
    })
    console.log(this.props.data.launchpad['state-details'])
    return (
      <header className="launchpadCard_header">
        <div className={"launchpadCard_header-title " + (this.props.isActive ? '' : 'launchpadCard_header-title-off')}>
          <a role="button"
            className="fleet-card-pwr-btn"
            style={{'color':this.props.isActive?'#93cb43':'white'}}
            onClick={this.setStatus.bind(this, this.props.name)}
            title={this.props.isActive ? 'Turn Off' : 'Turn On'}
            >
          <span className="oi" data-glyph="power-standby" aria-hidden="true"></span>
          </a>
          <h3 className="managementDomainCard_header-link" onClick={this.openDashboard} style={{cursor: this.props.isActive ? 'pointer' : 'default', 'text-overflow':'ellipsis', 'max-width':'150px', 'overflow':'hidden'}}>
            {this.props.name}
          </h3>
          <h3 className="managementDomainCard_header-link" style={{display: this.props.isActive ? 'inherit' : 'none'}}>
            <a onClick={this.openDashboard} title="Open Launchpad">
              <span className="oi" data-glyph="external-link" aria-hidden="true"></span>
            </a>
          </h3>
          <div className="managementDomainCard_header-actions">
            <h3>
              {isLoading || !isActive ? '' : 'Active' }
            </h3>
            <h3 style={{display: (isLoading || !isActive) ? 'none' : 'inherit'}}>
                <UpTime initialtime={startTime} run={true} />
            </h3>
            <h3 style={{display: isLoading ? 'none' : 'inherit'}}>
            </h3>
            <h3 className="launchpadCard_header-link">
                <a onClick={this.editMgmtDomain.bind(this, this.props.name)} title="Edit">
                  <span className="oi" data-glyph="pencil" aria-hidden="true">
                  </span>
                </a>
            </h3>
            <h3 className="launchpadCard_header-link" style={{display:'none'}}>
                <a onClick={this.openConsole} title="Open Console">
                  <span className="oi" data-glyph="monitor" aria-hidden="true">
                  </span>
                </a>
            </h3>
            <h3 className="launchpadCard_header-link" style={{display: this.props.isActive ? 'none' : 'inherit'}}>
              <a onClick={this.deleteLaunchpad.bind(this, this.props.name)} title="Delete">
                <span className="oi" data-glyph="trash" aria-hidden="true"></span>
              </a>
            </h3>
          </div>
        </div>
        <div className="managementDomainCard_header-status">
        </div>
        <LaunchpadOperationalStatus className="managementDomainCard_header-operational-status" loading={this.state.isLoading} doneLoading={this.doneLoading.bind(this)} display={this.state.displayStatus} currentStatus={this.props.data.launchpad.state} currentStatusDetails={this.props.data.launchpad['state-details']} />
      </header>
    )
  }
};
ManagementDomainCardHeader.propTypes = {
  name: React.PropTypes.string
 };
ManagementDomainCardHeader.defaultProps = {
  name: 'Loading...Some Name',
  isActive: false,
  data: {}
};


// <LaunchpadOperationalStatus className="managementDomainCard_header-operational-status" loading={isLoading} doneLoading={this.doneLoading.bind(this)} display={this.state.displayStatus} currentStatus={this.props.nsr["operational-status"]} status={this.props.nsr["rw-nsr:operational-events"]} />


export default ManagementDomainCardHeader;
