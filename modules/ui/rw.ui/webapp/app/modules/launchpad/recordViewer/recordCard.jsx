
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
import React from 'react';
import DashboardCard from '../../components/dashboard_card/dashboard_card.jsx';
import MonitoringParamsCarousel from '../../components/monitoring_params/monitoringParamsCarousel.jsx';
import * as Components from '../../components/components.js';
import VnfrCard from '../vnfr/vnfrCard.jsx';
import {LaunchpadCard, LpCardNfviMetrics, EpaParams, VnfrConfigPrimitives, NsrPrimitiveJobList} from '../launchpad_card/launchpadCard.jsx';
import NsrConfigPrimitives from '../launchpad_card/nsrConfigPrimitives.jsx';
import LoadingIndicator from '../../components/loading-indicator/loadingIndicator.jsx';
import NfviMetricBars from '../../components/nfvi-metric-bars/nfviMetricBars.jsx';
import ParseMP from '../../components/monitoring_params/monitoringParamComponents.js';
export default class RecordCard extends React.Component {
  constructor(props) {
    super(props)
  }
  render(){
    let html;
    let content;
    let card;
    let cardData = {};
    let components;
    let configPrimitivesProps = {};
    let displayConfigPrimitives = false;
    let configPrimitiveComponent = null;

    switch(this.props.type) {
      case 'vnfr' :
        cardData = this.props.data[0];
        // Disabling config primitives for VNF
        // configPrimitivesProps = [cardData];
        // displayConfigPrimitives = cardData['config-primitives-present'];
        // if (displayConfigPrimitives) {
        //   configPrimitiveComponent = <VnfrConfigPrimitives data={configPrimitivesProps} />;
        // }
        components = ParseMP.call(this, cardData["monitoring-param"], "vnfr-id");
        break;
      case 'nsr' :
        cardData = this.props.data.nsrs[0];
        configPrimitivesProps = cardData;
        displayConfigPrimitives = cardData['config-primitive'];
        if (displayConfigPrimitives) {
          configPrimitiveComponent = (
            <div className="flex nsConfigPrimitiveContainer">
              <NsrConfigPrimitives data={configPrimitivesProps} />
              <NsrPrimitiveJobList jobs={cardData['config-agent-job']}/>
            </div>
          );
        }
        components = ParseMP.call(this, cardData["monitoring-param"], "vnfr-id");
        break;
    }
    let mgmt_interface = cardData["dashboard-url"];
    let mgmtHTML;
    if(mgmt_interface) {
      mgmtHTML = <a href={mgmt_interface} target="_blank">Open Application Dashboard</a>;
    }
      if(this.props.isLoading) {
        html = <DashboardCard className="loading" showHeader={true} title={cardData["short-name"]}><LoadingIndicator size={10} show={true} /></DashboardCard>
      } else {
        let glyphValue = (this.props.recordDetailsToggleValue) ? "chevron-left" : "chevron-right";
        let nfviMetrics = this.props.type != 'vnfr' ? <LpCardNfviMetrics data={cardData["nfvi-metrics"]} /> : null;
        html = (
            <DashboardCard className="recordCard" showHeader={true} title={cardData["short-name"]}>
              <a onClick={this.props.recordDetailsToggleFn} className={"recordViewToggle " + (this.props.recordDetailsToggleValue ? "on": null)}><span className="oi" data-glyph={glyphValue} title="Toggle Details Panel" aria-hidden="true"></span></a>
              <div className="launchpadCard_title" style={{textAlign:'right'}}><span style={{float:'left'}}>MONITORING PARAMETERS</span>
                {mgmtHTML}
              </div>
              <div className="monitoringParams">{components}</div>
              <div className="nfvi-metrics">
                { nfviMetrics }
                <EpaParams data={cardData["epa-params"]} />
              </div>
              <div className="configPrimitives">
                {configPrimitiveComponent}
              </div>
              <div className="cardSectionFooter">
              </div>
            </DashboardCard>
        );
      }
    return html;
  }
}
RecordCard.defaultProps = {
  type: "default",
  data: {},
  isLoading: true
}
