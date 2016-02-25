
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
import React, {Component} from 'react';
import DashboardCard from '../../components/dashboard_card/dashboard_card.jsx';
import {Tab, Tabs, TabList, TabPanel} from 'react-tabs';
import CatalogItems from './catalogItems.jsx';
import NetworkServiceActions from './launchNetworkServiceActions.js';
//Data temporarily set through store and state for development
export default class SelectDescriptor extends Component {
  constructor(props) {
    super(props);
  }
  componentWillReceiveProps(nextProps) {
  }
  render() {
    return (
      <DashboardCard showHeader={true} title={'1. Select NSD'} className={'selectDescriptor'}>
        <Tabs>
          <TabList>
          </TabList>
          <TabPanel>
            <CatalogItems selectedNSDid={this.props.selectedNSDid} catalogs={this.props.nsd} onSelect={NetworkServiceActions.descriptorSelected} />
          </TabPanel>
        </Tabs>
      </DashboardCard>
    );
  }
}
SelectDescriptor.defaultProps = {
  nsd:[{

  }],
  vnfd:[{

  }],
  pnfd: [{

  }]
}

