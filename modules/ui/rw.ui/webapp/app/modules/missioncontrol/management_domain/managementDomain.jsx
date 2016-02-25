/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */

import React from 'react/addons';
import AppHeader from '../../components/header/header.jsx';
import AppHeaderActions from '../../components/header/headerActions.js';
var ManagementDomainStore = require('./managementDomainStore')
var ManagementDomainActions = require('./managementDomainActions')
class ManagementDomainAccount extends React.Component {
    constructor(props) {
        super(props);
        this.state = {
          store:ManagementDomainStore,
          mgmt: {},
          managementDomain: {}
        }
        ManagementDomainStore.listen(this.listener)
        ManagementDomainStore.getPools();
        if(this.props.edit) {
            ManagementDomainStore.getManagementDomain(window.location.hash.split('/')[2])
        }
    }

    // Returning to this funcitonality soon

    // selector(pools, type) {
    //   console.log(pools, type, this.state)
    //   if (!pools) {
    //     return;
    //   }
    //   pools.forEach(function(pool, idx) {
    //     console.log(pool, idx, this)
    //     if (pool['mgmt-domain']) {
    //       // assigned to another domain and unavailable
    //       pool.disabled = true;
    //     } else {
    //       pool.disabled = false;

    //       //Checks if pool has been selected. If not, then select that pool in the UI and set it in model to be passed to create function.
    //       if(!this.state[type + 'Pool']) {
    //         var firstPool = this.state[type + 'Pools'][idx];
    //         //firstPool.checked = true;
    //         var temp = {};
    //         temp[type + 'Pools'] = firstPool.name;
    //         this.setState(temp)
    //         //self.managementDomain['networkPool'] = "";
    //       }
    //       // Make others unchecked so we only have one checked
    //       // for (var i = 0; i < self[type + 'Pools'].length; i++) {
    //       //   if (idx != i) {
    //       //     self[type + 'Pools'][i].checked = false;
    //       //   }
    //       // }
    //     }
    //   }.bind(this));
    //   this.filterChange();
    // }
    // filterChange () {
    //   var firstPool = true;
    //   for (var i = 0; i < this.state.networkPools.length; i++) {
    //     var pool = this.state.networkPools[i]
    //     if (firstPool && !pool.disabled && pool['cloud-account'] === this.state.cloud_accounts_selector) {
    //       if (firstPool) {
    //         this.state.networkPool = pool.name;
    //         pool.checked = true;
    //         firstPool = false
    //       }
    //     } else {
    //       pool.checked = false;
    //     }
    //   }
    //   firstPool = true;
    //   for (var i = 0; i < self.vmPools.length; i++) {
    //     var pool = self.vmPools[i]
    //     if (firstPool && !pool.disabled && pool['cloud-account'] === self.cloud_accounts_selector) {
    //       if (firstPool) {
    //         self.managementDomain.vmPool = pool.name;
    //         pool.checked = true;
    //         firstPool = false
    //       }
    //     } else {
    //       pool.checked = false;
    //     }
    //   }
    // }
    listener = (data) => {
      var all = data.vmPools.concat(data.networkPools, data.portPools);
      var cloud_accounts = [];
      for (var i = 0; i < all.length; i++) {
        if (all[i] && cloud_accounts.indexOf(all[i]['cloud-account']) == -1) {
          cloud_accounts.push(all[i]['cloud-account']);
        }
      }
      this.setState({
        vmPools:data.vmPools || [],
        networkPools:data.networkPools || [],
        portPools:data.portPools || [],
        cloud_accounts_selector:cloud_accounts[0],
        cloud_accounts: cloud_accounts
      })

      // this.selector(this.state.vmPools, 'vm');
      // this.selector(this.state.networkPools, 'network');
      // this.selector(this.state.portPools, 'port');
    }
    filterPools(type) {
      if (this.state.cloud_accounts_selector === type['cloud-account']) {
        return true
      } else {
        return false;
      }

    }
    componentWillReceiveProps(nextProps) {}
    shouldComponentUpdate(nextProps) {
        return true;
    }
    handleNameChange(event) {
        var temp = this.state.mgmt;
        temp.name = event.target.value;
        this.setState({
            managementDomain: temp
        });
    }
    handleVMChange(event) {
        var temp = this.state.mgmt;
        temp.vm = event.target.value;
        this.setState({
            mgmt: temp
        })
    }
    handleNetworkChange(event) {
        var temp = this.state.mgmt;
        temp.network = event.target.value;
        this.setState({
            mgmt: temp
        })
    }
    handleCloudAccountsChange(event) {
        this.setState({
            cloud_accounts_selector: event.target.value
        })
    }
    update() {
      var self = this;
      console.log('self.managementDomain', self.state.managementDomain);
       ManagementDomainStore.update(self.state.managementDomain).then(function() {
        ManagementDomainStore.unlisten(self.listener);
        self.cancel();
       });
    };
    create() {
      var self = this;
      if (typeof(self.state.managementDomain.name) == "undefined" || self.state.managementDomain.name == "") {
        ManagementDomainActions.validateError('Please set name for Management Domain');
        return;
      }
      ManagementDomainActions.validateReset();
      console.log('self.managementDomain', self.managementDomain);
      console.log(self.state.managementDomain)
       ManagementDomainStore.create(self.state.managementDomain).then(function() {
        ManagementDomainStore.unlisten(self.listener);
        self.cancel();
       });
    };
    cancel() {
        let loc = window.location.hash.split('/');
        // hack to restore MC redirects from create cloud account page
        if (loc.indexOf('launchpad') == -1) {
            // this is a MC app URL
            if (loc.indexOf('edit') != -1) {
                // this is edit link
                loc.pop();
            }
            loc.pop();
            loc.pop()
            loc.push('/');
        } else {
            loc.pop();
        }
        window.location.hash = loc.join('/');
    }
    render() {


      var title = "Create Management Domain"
      var name = <input type="text" onChange={this.handleNameChange.bind(this)} style={{'text-align':'left'}} />
      var button = <a role="button" onClick={this.create.bind(this)} className="save">Save</a>
      if (this.props.edit) {
        var title = "Edit Management Domain"
        name = <label>{this.state.mgmt.name}</label>
        button = <a role="button" onClick={this.update.bind(this)} class="save">Update</a>

      }


      var cloud_accounts_dd = [];
      if (this.state.cloud_accounts && this.state.cloud_accounts.length > 0) {
        var cloud_accounts =[];
        for (var i = 0; i < this.state.cloud_accounts.length; i++) {
          var account = this.state.cloud_accounts[i];
          cloud_accounts.push(
            <option value={account}> {account} </option>
          )
        }

        cloud_accounts_dd = (
          <div>
            <label className="create-management-domain-filter">
            <span style={{margin:'5px 0px 0px 28px', float: 'left', position: 'relative'}}>Cloud Account:</span>
              <select selected={this.state.cloud_accounts_selector} style={{display:'inline-block', margin:'0px 0px 25px 0px'}} onChange={this.handleCloudAccountsChange.bind(this)}>
                {cloud_accounts}
              </select>
            </label>
          </div>
        )
      }

      var vmSelections = [];
      console.log(this.state)
      for (var i = 0; this.state.vmPools && i < this.state.vmPools.length; i++) {
        var pool = this.state.vmPools[i];
        if (this.filterPools(pool)) {
          vmSelections.push(
            <label>
                <input type="radio" onChange={this.handleNetworkChange.bind(this)} value={pool.name} disabled={pool.disabled} checked={pool.checked} /> {pool.name}
            </label>
          )
        }

      }
      if (vmSelections.length == 0) {
        vmSelections.push(
          <label>
            No VM pools added.
          </label>
        )
      }
      var netSelections = [];
      for (var i = 0; this.state.networkPools && i < this.state.networkPools.length; i++) {
        var pool = this.state.networkPools[i];
        if (this.filterPools(pool)) {
          netSelections.push(
            <label>
                <input type="radio" onChange={this.handleVMChange.bind(this)} value={pool.name} disabled={pool.disabled} checked={pool.checked} /> {pool.name}
            </label>
          )
        }

      }
      if (netSelections.length == 0) {
        netSelections.push(
          <label>
            No Network pools added.
          </label>
        )
      }

      var html = (
        <div>
          <AppHeader title={title} />
          <div className="app-body create">
            <screen-loader store={this.state.store}></screen-loader>
            <h2 className="create-management-domain-header name-input">
              {name}
            </h2>
            {cloud_accounts_dd}
            <ol className="flex-row">
              <li className="create-management-domain-pool">
                <h3> Associated VM Pool </h3>
                <div className="options flex-column">
                  {vmSelections}
                </div>
              </li>
              <li className="create-management-domain-pool">
                <h3> Associated Network Pool </h3>
                <div className="options flex-column">
                  {netSelections}
                </div>
              </li>
            </ol>
            <div className="form-actions">
                <a role="button" className="cancel" onClick={this.cancel}>Cancel</a>
                {button}
            </div>
          </div>
        </div>
      )
      return html;
    }
}
export default ManagementDomainAccount
