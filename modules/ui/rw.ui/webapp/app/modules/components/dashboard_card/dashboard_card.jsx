
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
import React from 'react';
import './dashboard_card.scss';

var cardClass = 'dashboardCard'//classSet(this.props.class);

var CardHeader = React.createClass({
  render() {
    var cardClassHeader = cardClass + '_header';
    if(this.props.className) {
        cardClassHeader += ' ' + this.props.className + '_header';
    }
   return (
    <header className={cardClassHeader}>
      <h3>
        {this.props.title}
      </h3>
    </header>
    )
  }
});
 CardHeader.defaultProps = {
  title: ' Loading...'
 }



var dashboardCard = React.createClass({
    componentDidMount: function() {

    },
    getDefaultProps: function() {
      return {
        isHidden: false
      }
    },
    render() {
      var cardClassWrapper = cardClass;
      var cardClassContent = cardClass + '_content';
      var cardClassContentBody = cardClassContent + '-body';
      var hasHeader;
      if(this.props.className) {
        cardClassWrapper += ' ' + this.props.className;
        cardClassContent += ' ' + this.props.className + '_content';
        cardClassContentBody += ' ' + this.props.className + '-body';
      }
    if (this.props.showHeader) {
      hasHeader = <CardHeader className={this.props.className} title={this.props.title}/>;
    };
    return (
        <div className={cardClassWrapper} style={{display: this.props.isHidden ? 'none':'flex'}}>
          <i className="corner-accent top left"></i>
          <i className="corner-accent top right"></i>
            {hasHeader}
            <div className={cardClassContent}>
              <div className={cardClassContentBody}>
                {this.props.children}
              </div>
            </div>
          <i className="corner-accent bottom left"></i>
          <i className="corner-accent bottom right"></i>
        </div>
      )
  }
})


// class DashboardCard extends React.Component {
//   constructor(props) {
//     super(props)
//   }
//   render() {

//   }
// }


export default dashboardCard;
