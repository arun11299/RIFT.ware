
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
import React from 'react';
import RecordViewActions from './recordViewActions.js';
import LoadingIndicator from '../../components/loading-indicator/loadingIndicator.jsx';
import DashboardCard from '../../components/dashboard_card/dashboard_card.jsx';
import '../network_service_launcher/catalog_items.scss';
import nsdImg from '../../../assets/img/network.svg';

export default class RecordNavigator extends React.Component{
  constructor(props) {
    super(props)
  }
  render(){
    let navClass = 'catalogItems';

    let self = this;
    let html;
    let className = this.props.isLoading ? 'loading' : '';
    let nav = [];

    this.props.nav.map(function(n, k) {
          let itemClassName = navClass + '_item';
                    let catalog_name = (n.type == 'nsr' ? <span>({n.nsd_name})</span> : '');
                    if(n.id == self.props.activeNavID) {
                      itemClassName += ' -is-selected';
                    }
                    let activeClass = n.id == self.props.activeNavID ? 'active':'';
                     nav.push(
                            <li key={'id' + k + '-' + n.id}  onClick={self.props.loadRecord.bind(self,n)} className={itemClassName} >
                              <img src={nsdImg} />
                              <section id={n.id}>
                              <h1 title={n.name}>{n.name}</h1>
                                <h2>{n.type}</h2>
                              </section>
                            </li>)
                  })
    if(this.props.isLoading) {
        html = <DashboardCard className="loading" showHeader={true} title="Loading..."><LoadingIndicator size={10} show={true} /></DashboardCard>
    } else {
        html = (
          <DashboardCard showHeader={true} title="Select Record" className={"recordNavigator" + className}>
            <ul className="catalogItems">
              {
                nav
              }
            </ul>
          </DashboardCard>
        );
    }
    return html;
  }
}
RecordNavigator.defaultProps = {
  nav: []
}


