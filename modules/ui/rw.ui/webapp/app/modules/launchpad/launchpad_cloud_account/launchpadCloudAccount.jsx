/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */

import React from 'react';
import AppHeader from '../../components/header/header.jsx';
import SdnAccountStore from '../../missioncontrol/sdn_account/createSdnAccountStore.js';
import CloudAccount from '../../missioncontrol/cloud_account/cloudAccount.jsx';
import CloudAccountStore from './cloudAccountStore';
import CloudAccountActions from './cloudAccountActions';
import AccountSidebar from '../account_sidebar/accountSidebar.jsx';

export default class LaunchpadCloudAccount extends React.Component {
    constructor(props) {
        super(props);
        this.state = CloudAccountStore.getState();
        this.state.sdnAccounts = [];
        SdnAccountStore.getSdnAccounts();
        CloudAccountStore.listen(this.updateState);
        SdnAccountStore.listen(this.updateSdnAccount);
        if(this.props.edit) {
            CloudAccountStore.getCloudAccount(window.location.hash.split('/')[4])
        } else {
            this.state.isLoading = false;
        }
    }
    updateSdnAccount = (data) => {
        let sdns = data.sdnAccounts || [];
        console.log(sdns);
        //[{"name":"test","account-type":"mock","mock":{"username":"test"}}]
        let toSend = [
            {
                "label" : "Select an SDN Account",
                "value": false
            }
        ]
        sdns.map(function(sdn) {
            sdn.label=sdn.name;
            sdn.value = sdn.name
            toSend.push(sdn);
        });

        this.setState({
            sdnAccounts: toSend
        })
    }
    updateState = (state) => {
        this.setState(state);
    }
    loadComposer = () => {
      let API_SERVER = rw.getSearchParams(window.location).api_server;
      let auth = window.sessionStorage.getItem("auth");
      let mgmtDomainName = window.location.hash.split('/')[2];
        window.location.replace('//' + window.location.hostname + ':9000/index.html?api_server=' + API_SERVER + '&upload_server=http://' + window.location.hostname + '&clearLocalStorage' + '&mgmt_domain_name=' + mgmtDomainName + '&auth=' + auth);
    }
    render() {
        let html;
        let body;
        let title = "Launchpad: Add Cloud Account";
        let mgmtDomainName = window.location.hash.split('/')[2];
        if (this.props.edit) {
            title = "Launchpad: Edit Cloud Account";
        }
        let navItems = [{
                name: 'DASHBOARD',
                href: '#/launchpad/' + mgmtDomainName
            },{
                name: 'CATALOG',
                'onClick': this.loadComposer
            },
            {
                name: 'Accounts'
            }
        ];
        console.log(navItems)
        if (this.props.isDashboard) {
            body = (<div>Edit or Create New Accounts</div>);
        } else {
             body = <CloudAccount {...this.props} store={CloudAccountStore} actions={CloudAccountActions} sdnAccounts={this.state.sdnAccounts} />
        }
        html = (<div>
                  <AppHeader title={title} nav={navItems} isLoading={this.state.isLoading} />
                    <div className="flex">
                      <AccountSidebar/>
                      {body}
                    </div>
              </div>);
        return html;
    }
}
