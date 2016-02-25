
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
import React from 'react/addons';
import LaunchpadCard from './launchpad_card/launchpadCard.jsx';
import AltContainer from 'alt/AltContainer';
import Loader from '../components/loading-indicator/loadingIndicator.jsx';
import ScreenLoader from '../components/screen-loader/screenLoader.jsx';
import MPFilter from './monitoring-params-filter.jsx'
import Crouton from 'react-crouton'
import Histogram from 'react-d3-histogram'
import AppHeader from '../components/header/header.jsx';
import Utils from '../utils/utils.js';

let ReactCSSTransitionGroup = React.addons.CSSTransitionGroup;
var LaunchpadFleetActions = require('./launchpadFleetActions.js');
var LaunchpadFleetStore = require('./launchpadFleetStore.js');

class LaunchpadApp extends React.Component {
  constructor(props) {
    super(props);
    var self = this;
    this.state = LaunchpadFleetStore.getState();
    this.state.slideno = 0;
    this.handleUpdate = this.handleUpdate.bind(this);

  }
  componentDidMount() {
  LaunchpadFleetStore.getCatalog();
  LaunchpadFleetStore.getLaunchpadConfig();
  LaunchpadFleetStore.listen(this.handleUpdate);

  // Can not put a dispatch here it causes errors
  // LaunchpadFleetActions.validateReset();
  setTimeout(function() {
    LaunchpadFleetStore.openNSRSocket();
  },100);

  Utils.detectInactivity(this.componentWillUnmount);
}
change(e) {

}
handleUpdate(data){
  this.setState(data);
}
closeError() {
  LaunchpadFleetActions.validateReset()
}
componentWillUnmount = () => {
  LaunchpadFleetStore.closeSocket();
}
loadComposer = () => {
  let API_SERVER = rw.getSearchParams(window.location).api_server;
  let auth = window.sessionStorage.getItem("auth");
  let mgmtDomainName = window.location.hash.split('/')[2];
  LaunchpadFleetStore.closeSocket();
  LaunchpadFleetStore.listen(this.handleUpdate);
  window.location.replace('//' + window.location.hostname + ':9000/index.html?api_server=' + API_SERVER + '&upload_server=http://' + window.location.hostname + '&clearLocalStorage' + '&mgmt_domain_name=' + mgmtDomainName + '&auth=' + auth);
};
render () {
    var self = this;
    let mgmtDomainName = window.location.hash.split('/')[2];
    let navItems = [{
      name: 'DASHBOARD',
      },{
      name: 'CATALOG(' + self.state.descriptorCount + ')',
      onClick: self.loadComposer
    }];
    if (this.state.isStandAlone) {
      navItems.push({
        name: 'ACCOUNTS',
        href: '#/launchpad/' + mgmtDomainName + '/cloud-account/dashboard',
        onClick: this.componentWillUnmount
      })
    };
    let nav = <AppHeader title={'LAUNCHPAD: ' + mgmtDomainName} nav={navItems} />
    return (
      <div>
      {nav}
        <ScreenLoader show={self.state.isLoading}/>
        <ReactCSSTransitionGroup
        transitionName="loader-animation"
        component="div"
        className="dashboardCard_wrapper"

        >
            {
              self.state.nsrs.map(
                (nsr, index) => {
                return  <LaunchpadCard deleting={nsr.deleting} slideno={this.state.slideno} key={nsr.id} id={nsr.id} name={nsr.name} data={nsr.data} nsr={nsr} isActive={nsr["admin-status"] == "ENABLED"} />
                }
              )
            }
          <LaunchpadCard create={true}/>
          </ReactCSSTransitionGroup>
        </div>
            )
  }
}
LaunchpadApp.propTypes = {
  // data: React.PropTypes.array,
  // name: React.PropTypes.string
 };
LaunchpadApp.defaultProps = {
  // name: 'Loading...',
  // data: []
};


export default LaunchpadApp
