
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
import React, {Component} from 'react';
import SelectDecriptor from './selectDescriptor.jsx';
import SLAParams from './specifySLAParameters.jsx';
import LaunchNetworkServiceStore from './launchNetworkServiceStore.js';
import LaunchNetworkServiceActions from './launchNetworkServiceActions.js';
import Button from '../../components/button/rw.button.js';
import './launchNetworkService.scss';
import Crouton from 'react-crouton'
import '../../styles/common.scss'
import '../../launchpad/launchpad.scss'
import Loader from '../../components/loading-indicator/loadingIndicator.jsx';
import ScreenLoader from '../../components/screen-loader/screenLoader.jsx';
import AppHeader from '../../components/header/header.jsx';
import AppHeaderActions from '../../components/header/headerActions.js';
import DashboardCard from '../../components/dashboard_card/dashboard_card.jsx';
let ReactCSSTransitionGroup = React.addons.CSSTransitionGroup;
let API_SERVER = rw.getSearchParams(window.location).api_server;

let editNameIcon = require("../../../assets/img/svg/launch-fleet-icn-edit.svg")

export default class LaunchNetworkService extends Component {
  constructor(props) {
    super(props);
    this.state = LaunchNetworkServiceStore.getState();
    this.state.validate = false;
    this.handleUpdate = this.handleUpdate.bind(this);
    this.updateName = this.updateName.bind(this);
    this.handleSave = this.handleSave.bind(this);
  }
  openLog() {
      var LaunchpadStore = require('../launchpadFleetStore.js')
      LaunchpadStore.getSysLogViewerURL('lp');
  }
  openAbout() {
    let loc = window.location.hash.split('/');
    loc.pop();
    LaunchNetworkServiceStore.resetView();
    loc.push('lp-about');
    window.location.hash = loc.join('/');
  }
  openDebug() {
    let loc = window.location.hash.split('/');
    loc.pop();
    LaunchNetworkServiceStore.resetView();
    loc.push('lp-debug');
    window.location.hash = loc.join('/');
  }
  componentDidMount() {
    LaunchNetworkServiceStore.listen(this.handleUpdate);
    LaunchNetworkServiceStore.getCatalog();
    LaunchNetworkServiceStore.getLaunchpadConfig();
    LaunchNetworkServiceStore.getCloudAccount(function() {
      LaunchNetworkServiceStore.getDataCenters();
    });
  }
  handleCancel() {
    let loc = window.location.hash.split('/');
    loc.pop();
    LaunchNetworkServiceStore.resetView();
    window.location.hash = loc.join('/');
  }
  handleUpdate(data) {
    this.setState(data);
  }
  isOpenMano = () => {
    return this.state.selectedCloudAccount['account-type'] == 'openmano';
  }
  updateName(event) {
    if (this.resetValidate) {
      this.setState({
        validate:{message:""},
        resetValidate: false
      });
    }
    let name = event.target.value;
    LaunchNetworkServiceStore.nameUpdated(name);
  }
  handleSave(launch) {
    if (this.state.name == "") {
        AppHeaderActions.validateError('Please name the network service')
      return;
    }
    if (!this.state.name.match(/^[a-zA-Z0-9_]*$/g)) {
      AppHeaderActions.validateError('Spaces and special characters except underscores are not supported in the network service name at this time');
      return;
    }
    if (this.isOpenMano() && (this.state.dataCenterID == "" || !this.state.dataCenterID)) {
      this.setState({
        validate: {
          message: "Please enter the Data Center ID"
        }
      });
      return;
    }
    LaunchNetworkServiceStore.resetView();
    LaunchNetworkServiceStore.saveNetworkServiceRecord(this.state.name, launch);
  }
  setValidate() {
    this.resetValidate = true;
  }
  handleSelectCloudAccount = (e) => {
    let cloudAccount = e;
    LaunchNetworkServiceStore.updateSelectedCloudAccount(cloudAccount);
  }
  handleSelectDataCenter = (e) => {
    let dataCenter = e;
    LaunchNetworkServiceStore.updateSelectedDataCenter(dataCenter);
  }
  inputParametersUpdated = (i, e) => {
    LaunchNetworkServiceStore.updateInputParam(i, e.target.value)
  }
  render() {
    let name = this.state.name;
    var self = this;
    let mgmtDomainName = window.location.hash.split('/')[2];
    let navItems = [{
      name: 'DASHBOARD',
      onClick: function() {
          window.history.back(-1);
        }
      },
      {
        'name': 'Instantiate'
      }
    ];
    let dataCenterID = this.state.dataCenterID;
    let isOpenMano = this.isOpenMano();
    let isStandAlone = this.state.isStandAlone;
    let showCrouton = this.state.validate && this.state.validate.message;
    let message = this.state.validate.message;
    let CloudAccountOptions = [];
    let DataCenterOptions = [];
    let thirdPanel = null;
    //Build CloudAccountOptions
    CloudAccountOptions = this.state.cloudAccounts.map(function(ca, index) {
      return {
        label: ca.name,
        value: ca
      }
    });
    if (isOpenMano) {
        //Build DataCenter options
        let dataCenter = this.state.dataCenters[this.state.selectedCloudAccount.name];
        if(dataCenter){
            DataCenterOptions = dataCenter.map(function(dc, index) {
              return {
                label: dc.name,
                value: dc.uuid
              }
            });
        }
      }
    if (this.state.hasConfigureNSD) {
      thirdPanel = (
        <DashboardCard showHeader={true} title={'3. Configure NSD'} className={'configure-nsd'}>
          {
            this.state['input-parameters'].map(function(input, i) {
              return (
                      <label key={i}>
                        <span> { input.label || input.xpath } </span>
                        <input type="text" onChange={self.inputParametersUpdated.bind(self, i)} />
                      </label>
              )
            })
          }
        </DashboardCard>
      );
    }
    let html = (
      <div className={"app-body create-fleet"}>
      <ScreenLoader show={this.state.isLoading}/>
      <AppHeader title={'Launchpad: Instantiate Network Service'} nav={navItems} />
        <h2 className="create-fleet-header name-input">
           <label>Name <input type="text" pattern="^[a-zA-Z0-9_]*$" style={{textAlign:'left'}} onChange={this.updateName} value={name}/></label>
        </h2>
        <h2 className="create-fleet-header name-input" style={{display: isStandAlone ? 'inherit' : 'none'}}>
          <label>Select Cloud Account
            <SelectOption className="create-fleet-header" options={CloudAccountOptions} onChange={this.handleSelectCloudAccount} />
          </label>

        </h2>
        <h2 className="create-fleet-header name-input" style={{display: isOpenMano ? 'inherit' : 'none'}}>
          <label>Data Center
            <SelectOption className="create-fleet-header" options={DataCenterOptions} onChange={this.handleSelectDataCenter} />
          </label>
        </h2>
        <div className="launchNetworkService">
          <div className="dashboardCard_wrapper launchNetworkService_panels">
            <SelectDecriptor nsd={this.state.nsd} vnfd={this.state.vnfd} selectedNSDid={this.state.selectedNSDid}/>
            <SLAParams data={this.state.sla_parameters}/>
            {thirdPanel}
          </div>
        </div>
        <div className="launchNetworkService_controls">
            <Button label="Cancel" onClick={this.handleCancel} className="light"/>
            <Button label="Save" onClick={this.handleSave.bind(this, false)} className="light" loadingColor="black" isLoading={this.state.isSaving}/>
            <Button label="Launch" isLoading={this.state.isSaving} onClick={this.handleSave.bind(this, true)} className="dark"/>
        </div>
      </div>
    )
    return html
  }
}

class SelectOption extends React.Component {
  constructor(props){
    super(props);
  }
  handleOnChange = (e) => {
    this.props.onChange(JSON.parse(e.target.value));
  }
  render() {
    let html;
    html = (
      <select className={this.props.className} onChange={this.handleOnChange}>
        {
          this.props.options.map(function(op, i) {
            return <option key={i} value={JSON.stringify(op.value)}>{op.label}</option>
          })
        }
      </select>
    );
    return html;
  }
}
SelectOption.defaultProps = {
  options: [],
  onChange: function(e) {
    console.dir(e)
  }
}
