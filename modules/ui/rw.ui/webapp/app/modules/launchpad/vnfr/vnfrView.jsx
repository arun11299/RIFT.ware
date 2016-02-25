
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
import React from 'react/addons';
import VnfrActions from './vnfrActions.js';
import VnfrSource from './vnfrSource.js';
import VnfrStore from './vnfrStore.js';
import VnfrCard from './vnfrCard.jsx';
import Loader from '../../components/loading-indicator/loadingIndicator.jsx';
import ScreenLoader from '../../components/screen-loader/screenLoader.jsx';
let ReactCSSTransitionGroup = React.addons.CSSTransitionGroup;
class VnfrView extends React.Component {
  constructor(props) {
    super(props);
    this.state = VnfrStore.getState();
    this.state.vnfrs = [];
  }
  componentDidMount() {
    VnfrStore.listen(this.handleUpdate);
    console.log(VnfrStore)
    setTimeout(function() {
      VnfrStore.openVnfrSocket();
      // VnfrStore.vnfrMock();
    },100);
  }
  handleUpdate = (data) => {
    this.setState(data);
  }
  render() {
    let self = this;
    let html;
    let vnfrCards = [];
    self.state.vnfrs.map(function(vnfr) {
                 vnfrCards.push(<VnfrCard data={vnfr}></VnfrCard>)
    })
    html = (
            <ReactCSSTransitionGroup
            transitionName="loader-animation"
            component="div"
            className="dashboardCard_wrapper"

            >
              {vnfrCards}
            </ReactCSSTransitionGroup>
    );
    return html;
  }
}
export default VnfrView;
