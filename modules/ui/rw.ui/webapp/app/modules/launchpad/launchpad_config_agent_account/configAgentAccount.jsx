/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */

import React from 'react/addons';
import AppHeaderActions from '../../components/header/headerActions.js';
import './configAgentAccount.scss';
var ConfigAgentAccountStore = require('./configAgentAccountStore')
var ConfigAgentAccountActions = require('./configAgentAccountActions')
class ConfigAgentAccount extends React.Component {
    constructor(props) {
        super(props);
        ConfigAgentAccountStore.resetAccount();
        this.state = ConfigAgentAccountStore.getState();
        ConfigAgentAccountStore.listen(this.storeListener);
    }
    storeListener = (state) => {
        this.setState(
            {
                configAgentAccount: {
                    name:state.configAgentAccount.name,
                    "account-type":state.configAgentAccount["account-type"],
                    params:state.configAgentAccount[state.configAgentAccount["account-type"]]
                },
                isLoading: state.isLoading
            }
        )
    }
    create() {
        var self = this;
        if (self.state.configAgentAccount.name == "") {
            AppHeaderActions.validateError("Please give the config agent account a name");
            return;
        } else {
            var type = self.state.configAgentAccount['account-type'];
            if (typeof(self.state.params[type]) != 'undefined') {
                var params = self.state.params[type];
                for (var i = 0; i < params.length; i++) {
                    var param = params[i].ref;
                    if (typeof(self.state.configAgentAccount[type]) == 'undefined' || typeof(self.state.configAgentAccount[type][param]) == 'undefined' || self.state.configAgentAccount[type][param] == "") {
                        if (!params[i].optional) {
                            AppHeaderActions.validateError("Please fill all account details");
                            return;
                        }
                    }
                }
            }
        }
        ConfigAgentAccountActions.validateReset();
        ConfigAgentAccountStore.create(self.state.configAgentAccount).then(function() {
            let loc = window.location.hash.split('/');
            loc.pop();
            // hack to restore MC redirects from create config agent account page
            if (loc.indexOf('launchpad') == -1) {
                // this is a MC app URL
                loc.pop();
                loc.push('/');
            }
            loc.push('dashboard');
            window.location.hash = loc.join('/');
        });
    }
    update() {
        var self = this;

        if (self.state.configAgentAccount.name == "") {
            AppHeaderActions.validateError("Please give the config agent account a name");
            return;
        }

          var submit_obj = {
            'account-type':self.state.configAgentAccount['account-type'],
            name:self.state.configAgentAccount.name
          }
          submit_obj[submit_obj['account-type']] = self.state.configAgentAccount[submit_obj['account-type']];
          if(!submit_obj[submit_obj['account-type']]) {
            submit_obj[submit_obj['account-type']] = {};
          }
          for (var key in self.state.configAgentAccount.params) {
            if (submit_obj[submit_obj['account-type']][key]) {
                //submit_obj[submit_obj['account-type']][key] = self.state.configAgentAccount.params[key];
                console.log('hold')
            } else {
                submit_obj[submit_obj['account-type']][key] = self.state.configAgentAccount.params[key];
            }
          }

         ConfigAgentAccountStore.update(submit_obj).then(function() {
            // hack to restore MC app URL
            let loc = window.location.hash.split('/');
            if (loc.indexOf('launchpad') == -1) {
                // this is a MC app URL
                loc.pop();
                loc.pop();
                loc.pop();
                loc.push('/');
                window.location.hash = loc.join('/');
            } else {
                ConfigAgentAccountStore.unlisten(self.storeListener);
                self.cancel();
            }
         });
    }
    cancel() {
        let loc = window.location.hash.split('/');
        // hack to restore MC redirects from create config agent account page
        if (loc.indexOf('launchpad') == -1) {
            // this is a MC app URL
            if (loc.indexOf('edit') != -1) {
                // this is edit link
                loc.pop();
            }
            loc.pop();
            loc.pop();
            loc.push('/');
        } else {
            if (loc.indexOf('edit') != -1) {
                // this is edit link
                loc.pop();
                loc.pop();
                loc.push('dashboard');
            } else {
                loc.pop();
                loc.push('dashboard');
            }
        }
        window.location.hash = loc.join('/');
    }
    componentWillReceiveProps(nextProps) {}
    shouldComponentUpdate(nextProps) {
        return true;
    }
    handleDelete = () => {
        let self = this;
        ConfigAgentAccountStore.delete(self.state.configAgentAccount.name, function() {
            // hack to restore MC app URL
            let loc = window.location.hash.split('/');
            if (loc.indexOf('launchpad') == -1) {
                // this is a MC app URL
                loc.pop();
                loc.pop();
                loc.pop();
                loc.push('/');
                window.location.hash = loc.join('/');
            } else {
                self.cancel();
            }
        });
    }
    handleNameChange(event) {
        var temp = this.state.configAgentAccount;
        temp.name = event.target.value;
        this.setState({
            configAgentAccount: temp
        });
    }
    handleAccountChange(node, event) {
        var temp = this.state.configAgentAccount;
        temp['account-type'] = event.target.value;
        this.setState({
            configAgentAccount: temp
        })
    }
    handleParamChange(node, event) {
        var temp = this.state.configAgentAccount;
        if (!this.state.configAgentAccount[this.state.configAgentAccount['account-type']]) {
            temp[temp['account-type']] = {}
        }
        temp[temp['account-type']][node.ref] = event.target.value;
        this.setState({
            configAgentAccount: temp
        });
    }

    render() {
        // This section builds elements that only show up on the create page.
        var name = <label>Name <input type="text" onChange={this.handleNameChange.bind(this)} style={{'text-align':'left'}} /></label>
        var buttons = [
            <a role="button" onClick={this.cancel} class="cancel">Cancel</a>,
            <a role="button" onClick={this.create.bind(this)} className="save">Save</a>
        ]
        if (this.props.edit) {
            name = <label>{this.state.configAgentAccount.name}</label>
            var buttons = [
                <a role="button" onClick={this.handleDelete} ng-click="create.delete(create.configAgentAccount)" className="delete">Remove Account</a>,
                    <a role="button" onClick={this.cancel} class="cancel">Cancel</a>,
                    <a role="button" onClick={this.update.bind(this)} className="update">UPdate</a>
            ]
            let selectAccount = null;
            let params = null;
        }
        // This creates the create screen radio button for account type.
        var selectAccountStack = [];
        if (!this.props.edit) {
            for (var i = 0; i < this.state.accountType.length; i++) {
                var node = this.state.accountType[i];
                selectAccountStack.push(
                  <label>
                    <input type="radio" name="account" onChange={this.handleAccountChange.bind(this, node)} defaultChecked={node.name == this.state.accountType[0].name} value={node['account-type']} /> {node.name}
                  </label>
                )

            }
            var selectAccount = (
                <div className="select-type" style={{"margin-left":"27px"}}>
                    Select Account Type:
                    {selectAccountStack}
                </div>
            )
        }



        // This sections builds the parameters for the account details.
        var params = null;
        if (this.state.params[this.state.configAgentAccount['account-type']]) {
            var paramsStack = [];
            for (var i = 0; i < this.state.params[this.state.configAgentAccount['account-type']].length; i++) {
                var optionalField = '';
                var node = this.state.params[this.state.configAgentAccount['account-type']][i];
                var value = ""
                if (this.state.configAgentAccount[this.state.configAgentAccount['account-type']]) {
                    value = this.state.configAgentAccount[this.state.configAgentAccount['account-type']][node.ref]
                }
                if (this.props.edit && this.state.configAgentAccount.params) {
                    value = this.state.configAgentAccount.params[node.ref];
                }

                // If you're on the edit page, but the params have not been recieved yet, this stops us from defining a default value that is empty.
                if (this.props.edit && !this.state.configAgentAccount.params) {
                    break;
                }
                if (node.optional) {
                    optionalField = <span className="optional">Optional</span>;
                }
                paramsStack.push(
                    <label key={i}>
                      <label className="create-fleet-pool-params">{node.label} {optionalField}</label>
                      <input className="create-fleet-pool-input" type="text" onChange={this.handleParamChange.bind(this, node)} defaultValue={value}/>
                    </label>
                );
            }

            params = (
                <li className="create-fleet-pool">
              <h3> Enter Account Details</h3>
              {paramsStack}
          </li>
            )
        } else {
            params = (
                <li className="create-fleet-pool">
              <h3> Enter Account Details</h3>
              <label style={{'margin-left':'17px', color:'#888'}}>No Details Required</label>
          </li>
            )
        }

        // This section builds elements that only show up in the edit page.
        if (this.props.edit) {
            name = <label>{this.state.configAgentAccount.name}</label>
            var buttons = [
                <a role="button" onClick={this.handleDelete} ng-click="create.delete(create.configAgentAccount)" className="delete">Remove Account</a>,
                    <a role="button" onClick={this.cancel} class="cancel">Cancel</a>,
                    <a role="button" onClick={this.update.bind(this)} className="update">Update</a>
            ]
            let selectAccount = null;
            let params = null;
        }

        var html = (

              <div className="app-body create configAgentAccount">
                  <h2 className="create-management-domain-header name-input">
                       {name}
                  </h2>
                  {selectAccount}
                  <ol className="flex-row">
                      {params}
                  </ol>
                  <div className="form-actions">
                      {buttons}
                  </div>
              </div>
        )
        return html;
    }
}
export default ConfigAgentAccount
