/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */

import React from 'react';
import AppHeader from '../../components/header/header.jsx';
import CloudAccount from './cloudAccount.jsx';
import CloudAccountStore from './cloudAccountStore';
import CloudAccountActions from './cloudAccountActions';

export default class CloudAccountApp extends React.Component {
    constructor(props) {
        super(props);
        this.state = CloudAccountStore.getState();
        CloudAccountStore.listen(this.updateState);
        if(this.props.edit) {
            CloudAccountStore.getCloudAccount(window.location.hash.split('/')[2])
        } else {
            this.state.isLoading = false;
        }
    }
    updateState = (state) => {
        this.setState(state);
    }
    render() {
        let html;
        let title = "Add Cloud Account";
        if (this.props.edit) {
            title = "Edit Cloud Account";
        }
        let navItems = [{
          name: 'DASHBOARD',
          href: '#/'
        }];

        html = (<div>
                  <AppHeader title={title} nav={navItems} isLoading={this.state.isLoading} store={CloudAccountStore} actions={CloudAccountActions} />
                  <CloudAccount {...this.props} />
              </div>);
        return html;
    }
}
