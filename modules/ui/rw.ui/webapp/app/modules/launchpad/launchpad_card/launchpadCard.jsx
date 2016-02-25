
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
import React from 'react/addons';
import DashboardCard from '../../components/dashboard_card/dashboard_card.jsx';
import './launchpad_card.scss';
import LaunchpadNSInfo from './launchpadNSInfo.jsx';
import LaunchpadHeader from './launchpadHeader.jsx';
import MonitoringParamsCarousel from '../../components/monitoring_params/monitoringParamsCarousel.jsx';
import LaunchpadControls from './launchpadControls.jsx';
import NfviMetricBars from '../../components/nfvi-metric-bars/nfviMetricBars.jsx';
import LoadingIndicator from '../../components/loading-indicator/loadingIndicator.jsx';
import RecordViewStore from '../recordViewer/recordViewStore.js';
import TreeView from 'react-treeview';

function openLaunch() {
  window.location.hash = window.location.hash + '/launch';
}

class LaunchpadCard extends React.Component {
  constructor(props) {
    super(props);
  }
  shouldComponentUpdate(nextProps) {
    return true;
  }
  render() {
    let html;
    let metrics = '';
    let monitoring_params_data;
    let deleting = false;

    if(this.props.nsr && this.props.nsr.data) {
      metrics = this.props.nsr.data.map((info, index)=> {
          return (<LaunchpadNSInfo  key={index} name={info.name} data={info.data}/>)
      });
    }
    // debugger
    if(this.props.nsr && this.props.nsr["monitoring-param"]) {
      monitoring_params_data = this.props.nsr["monitoring-param"];
    } else {
      monitoring_params_data = null;
    }

    if (this.props.nsr && this.props.nsr.deleting) {
      deleting = true;
    }

    if(this.props.create){
      html = <DashboardCard> <div className={'launchpadCard_launch'} onClick={openLaunch} style={{cursor:'pointer'}}><img src={require("../../../assets/img/launchpad-add-fleet-icon.png")}/> Instantiate Network Service </div> </DashboardCard>;
    } else {
      html = (
        <DashboardCard className={'launchpadCard'}>
          <LaunchpadHeader nsr={this.props.nsr} name={this.props.name} isActive={this.props.isActive} id={this.props.id}/>
          {
          deleting ?
          <div className={'deletingIndicator'}>
            <LoadingIndicator size={10} show={true} />
          </div>
          :
          <div className="launchpadCard_content">
            <div className="launchpadCard_title">
              NSD: {this.props.nsr.nsd_name}
            </div>
            <MonitoringParamsCarousel component_list={monitoring_params_data} slideno={this.props.slideno}/>
            <LaunchpadControls controlSets={this.props.nsr.nsControls} />
            <LpCardNfviMetrics name="NFVI METRICS" data={this.props.nsr["nfvi-metrics"]} />
            <EpaParams data={this.props.nsr["epa-params"]} />
          </div>
          }
        </DashboardCard>
      );
    }
    return html;
  }
}
LaunchpadCard.propTypes = {
  nsr: React.PropTypes.object,
  isActive: React.PropTypes.bool,
  name: React.PropTypes.string
 };
LaunchpadCard.defaultProps = {
  name: 'Loading...',
  data: {},
  isActive: false
};

export class LpCardNfviMetrics extends React.Component {
  constructor(props) {
    super(props)
  }
  render() {
    let html = (
      <div style={{overflow: 'hidden'}}>
        <div className="launchpadCard_title">
          NFVI-METRICS
        </div>
        { (this.props.data && this.props.data.length > 0) ? <NfviMetricBars metrics={this.props.data}/> : <div className="empty"> NO NFVI METRICS CONFIGURED</div>}
      </div>
    )
    return html;
  }
}
export class EpaParams extends React.Component {
  constructor(props) {
    super(props)
  }
  render() {
    let metrics =[];
    let epa = this.props.data;
    for(let k in epa) {
      let epaHTMLsub = [];
      let epaHTML;
      epaHTMLsub.push(buildParams(epa[k]));
      epaHTML = (
        <li>
          <h1>{k}</h1>
          {
            epaHTMLsub
          }
        </li>
      );
      metrics.push(epaHTML)
    }
    function buildParams(epa) {
      let html = [];
      let checkForTotal = function checkForTotal(epa, html) {
        for (let k in epa) {
          if("total" in epa[k]) {
            html.push(<dd>{k} : {epa[k].total} {(epa[k].total>1) ? 'vms' : 'vm'}</dd>)
          } else {
            html.push(<dt>{k}</dt>)
            checkForTotal(epa[k], html)
          }
        }
      }
      checkForTotal(epa, html)
      return (<dl>{html}</dl>);
    }
    let display = (<ul>
              {metrics}
            </ul>)
    // metrics = false;
    if(metrics.length == 0) display = <div className="empty">NO EPA PARAMS CONFIGURED</div>
    let html = (
      <div style={{overflow: 'hidden'}}>
        <div className="launchpadCard_title">
          EPA-PARAMS
        </div>
        <div className={"launchpadCard_data-list" + ' ' + 'EPA-PARAMS'}>
          {display}
        </div>
      </div>
    )
    return html;
  }
}
export class VnfrConfigPrimitives extends React.Component {
  constructor(props) {
    super(props);
    this.state = {
      vnfrs: []
    };
    this.state.execPrimitive = {};
  }
  handleExecuteClick = (configPrimitiveIndex, vnfrIndex, e) => {
    RecordViewStore.constructAndTriggerVnfConfigPrimitive({
      vnfrs: this.state.vnfrs,
      configPrimitiveIndex: configPrimitiveIndex,
      vnfrIndex: vnfrIndex
    });
  }
  handleParamChange = (paramIndex, configPrimitiveIndex, vnfrIndex, e) => {
    let vnfrs = this.state.vnfrs;
    vnfrs[vnfrIndex]["vnf-configuration"]["config-primitive"][configPrimitiveIndex]["parameter"][paramIndex].value = e.target.value
    this.setState({
      vnfrs: vnfrs
    })
  }
  componentWillReceiveProps(props) {
    if (this.state.vnfrs.length == 0) {
      this.setState({
        vnfrs: props.data
      })
    }
  }
  render() {
    let configPrimitives = [];
    this.state.vnfrs.map((vnfr, vnfrIndex) => {
      if (vnfr['vnf-configuration'] && vnfr['vnf-configuration']['config-primitive'] && vnfr['vnf-configuration']['config-primitive'].length > 0) {
        vnfr['vnf-configuration']['config-primitive'].map((configPrimitive, configPrimitiveIndex) => {
          let params = [];
          if (configPrimitive['parameter'] && configPrimitive['parameter'].length > 0) {
            configPrimitive['parameter'].map((param, paramIndex) => {
              let optionalField = '';
              if (param.mandatory == 'false') {
                optionalField = <span className="optional">Optional</span>
              }
              params.push(
                <div>
                  <label className="">{param.name}</label>
                  <input className="" type="text" value={param.value} onChange={this.handleParamChange.bind(this, paramIndex, configPrimitiveIndex, vnfrIndex)}> </input>
                  {optionalField}
                </div>
              );
            });
          }

          configPrimitives.push(
            <div>
              {params}
              <button className="" role="button" onClick={this.handleExecuteClick.bind(this, configPrimitiveIndex, vnfrIndex)}>{configPrimitive.name}</button>
            </div>
          )
        });
      }
    });

    let vnfName = this.state.vnfrs[0] && this.state.vnfrs[0]['short-name'] ? this.state.vnfrs[0]['short-name'] : null;
    let display = configPrimitives;
    let html = (
      <div style={{overflow: 'hidden'}}>
        <div className="launchpadCard_title">
          CONFIG-PRIMITIVES
        </div>
        <div className={"launchpadCard_data-list" + ' ' + 'CONFIG-PRIMITIVES'}>
          <div>Config actions for {vnfName}:</div>
          {display}
        </div>
      </div>
    )
    return html;
  }
}

export class NsrPrimitiveJobList extends React.Component {
  constructor(props) {
    super(props)
  }
  render () {
    let tree = null;
    tree = this.props.jobs.map(function(job, jindex) {
          let nsrJobLabel = "NSR Job ID:" + job['job-id']
          return (
              <TreeView key={jindex} nodeLabel={nsrJobLabel} className="nsrJob">
                <h4>NSR Job Status: {job['job-status']}</h4>
                  {job.vnfr ?
                  <TreeView defaultCollapsed={true} className="vnfrJobs" nodeLabel="VNFR Jobs">
                    {job.vnfr.map(function(v, vindex) {
                      return (
                        <TreeView key={vindex} nodeLabel="VNFR Job">
                          <h5>VNFR ID: {v.id}</h5>
                          <h5>VNFR Job Status: {v['vnf-job-status']}</h5>
                            {v.primitive.map((p, pindex) => {
                              return (
                                <div key={pindex}>
                                  <div>Name: {p.name}</div>
                                  <div>Status: {p["execution-status"]}</div>
                                </div>
                              )
                            })}
                        </TreeView>
                      )
                    })}
                  </TreeView>
                  :null}
              </TreeView>
          )
        });
    let html = (
      <div  className="nsConfigPrimitiveJobList">
        <div className="launchpadCard_title">
          JOB-LIST
        </div>
        <div className="listView">
          <div>
            {tree}
          </div>
        </div>
      </div>
    );
    return html;
  }
}
NsrPrimitiveJobList.defaultProps = {
  jobs: []
}

export default LaunchpadCard
