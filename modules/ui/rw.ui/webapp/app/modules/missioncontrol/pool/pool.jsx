import React from 'react/addons';
import AppHeader from '../../components/header/header.jsx';
var poolStore = require('./poolStore.js')
var poolActions = require('./poolActions.js')
class Pool extends React.Component {
  constructor(props) {
    super(props);
    var self = this;
    poolStore.listen(function(state) {
      self.setState(state)
    })
  }
  init = () => {

  }
  componentWillMount() {
    console.log('willmount');
    var self = this;
    poolStore.requestPage()
    this.state = poolStore.getState();
    this.state.selectedResources = [];
    let cloudAccount = self.state.pool["cloud-account"];
    if(!this.state.isCreatePage) {
      poolStore.pools(this.state.pool.type, this.state.pool.name);
    } else  {
      poolStore.resources(cloudAccount);
    }
  }
  save = () => {
    let pool = this.state.pool;
    let assigned = [];
    this.state.resources[pool.type].map(function(r, i) {
      if(r.selected) {
        assigned.push({
          id: r.id
        });
      }
    })
    pool.assigned = assigned
    if (this.state.isCreatePage) {
      poolStore.create(pool);
    } else {
      poolStore.edit(pool)
    }
  }
  deletePool = () => {
    let pool = this.state.pool;
    if (!this.state.isCreatePage) {
      poolStore.delete(pool);
    }
  }
  componentDidMount() {
    console.log('pool mounted');
  }
  componentWillReceiveProps(nextProps) {
    console.log('pool received props');
  }
  shouldComponentUpdate(nextProps) {
    return true;
  }
  updatePoolType = (e) => {
    poolStore.updatePoolType(e.target.value)
  }
  updateDynamicScaling = (e) => {
    poolStore.updateDynamicScaling(e.target.value)
  }
  addSelectedResources = (resources) => {
    poolStore.addSelectedResources(resources);
  }
  addAllResources = () => {
    poolStore.addAllResources();
  }
  handleSelectedResource = (resource, e) => {
    resource.checked = e.target.checked
  }
  handleNameUpdate = (e) => {
    poolStore.updateName(e.target.value);
  }
  removeAllResources = () => {
    poolStore.removeAllResources();
  }
  removeSelectedResources = (resources) => {
     poolStore.removeSelectedResources(resources);
  }
  render() {
    let self = this;
    let poolType = this.state.pool.type;
    let poolName = this.state.pool.name;
    let resources = this.state.resources;
    let isDynamic = this.state.pool['dynamic-scaling'];
    let createShow = {'display': this.state.isCreatePage ? 'inherit' : 'none'};
    let createHide = {'display': this.state.isCreatePage ? 'none' : 'inherit'}
    let availableResources = [];
    let selectedResources = [];

    let nav = <AppHeader title={this.state.isCreatePage ? 'Create Pool' : 'Edit Pool'} isLoading={this.state.isLoading} />
    //Consolidate these resource functions
    if(poolType != "") {
      availableResources = resources[poolType].map(function(d, i) {
      if(d.available && !d.selected) {
        return (
          <label>
            <input style={{float:'left', position:'relative', width:'100%'}} type="checkbox" onChange={self.handleSelectedResource.bind(self, d)} /> {d.name}
          </label>
        )
      }
      });
      selectedResources = this.state.resources[poolType].map(function(d, i) {
        if(d.available && d.selected) {
          return (
            <label>
              <input style={{float:'left', position:'relative', width:'100%'}} type="checkbox" onChange={self.handleSelectedResource.bind(self, d)} /> {d.name}
            </label>
          )
        }
      });
    }
    var html = (
      <div>
      {nav}
      <div className="app-body create">

        <h2 className="create-fleet-header name-input" >
           <label>Name <input type="text" value={poolName} onChange={this.handleNameUpdate} style={{textAlign:'left'}} />
           </label>
        </h2>
        <br/>

        <div className="select-pool-type"  style={createShow}>
          Select Pool Type:
          <label>
            <input type="radio" name="poolType" checked={poolType == 'vm'}  onChange={this.updatePoolType} value="vm" /> VM
          </label>
          <label>
            <input type="radio" name="poolType" checked={poolType == 'network'} onChange={this.updatePoolType}  value="network" /> Network
          </label>
        </div>
        <div className="display-pool-type"  style={createHide}>
          Pool Type: { poolType }
        </div>

        <ol className="flex-row create-pool-container">
          <li className="create-fleet-dynamic">
            <h3>Dynamic Resources</h3>
            <div className="create-pool-dynamic-radio flex-row">
              <label className="create-pool-dynamic-input">
                <input type="radio" name="dynamic-resourcs" onChange={this.updateDynamicScaling}  checked={isDynamic == true}  value='true' /> Yes
              </label>
              <label className="create-pool-dynamic-input">
                <input type="radio" name="dynamic-resourcs" checked={isDynamic == false}  onChange={this.updateDynamicScaling} value='false' /> No
              </label>
            </div>
          </li>
        </ol>

        <ol className="flex-row create-pool-container">
          <li className="create-fleet-pool">
            <h3>AVAILABLE STATIC {poolType.toUpperCase()} RESOURCES</h3>
            <div className="create-fleet-selectors">
              <button onClick={this.addSelectedResources.bind(self, resources)}> Add Selected </button>
              <button onClick={this.addAllResources}> Add All </button>
            </div>
            <div className="options flex-row">
            {
              availableResources ? availableResources : (<label ng-hide="create.store.state.isLoading || create.availableNetworkResources()">No resources Available</label>)
            }
            </div>
          </li>

          <li className="create-fleet-services" ng-show="create.pool.type == 'vm'">
            <h3>SELECTED STATIC {poolType.toUpperCase()} RESOURCES</h3>
            <div className="create-fleet-selectors">
              <button onClick={this.removeSelectedResources.bind(self, resources)}> Remove Selected </button>
              <button onClick={this.removeAllResources}> Remove All </button>
            </div>
            <div className="options flex-row">
            {
              selectedResources ? selectedResources : (<label ng-hide="create.store.state.isLoading || create.availableNetworkResources()">No resources Available</label>)
            }
            </div>
          </li>

        </ol>
       <div className="form-actions">
        <a role="button" onClick={this.deletePool}  className="delete">Delete Pool</a>
          <a role="button" ng-click="create.cancel()" className="cancel">Cancel</a>
          <a role="button" onClick={this.save} className="save">Save</a>
        </div>
      </div>
    </div>
      )
    return html;
  }
}
export default Pool
