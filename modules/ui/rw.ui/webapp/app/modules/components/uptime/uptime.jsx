
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
/**
 * Uptime Compoeent
 * Accepts two properties:
 * initialtime {number} number of milliseconds.
 * run {boolean} determines whether the uptime ticker should run
 */

import React from 'react';

class UpTime extends React.Component {
    constructor(props) {
      super(props);
      this.state = {
        run: props.run,
        time: this.handleConvert(props.initialtime)
      }
      this.tick;
      this.handleConvert = this.handleConvert.bind(this);
      this.updateTime = this.updateTime.bind(this);
    }
    componentWillReceiveProps(nextProps) {
      var self = this;
      this.setState({
         time: self.handleConvert(nextProps.initialtime)
      })
    }
    handleConvert(input) {
      var ret = {
        days: 0,
        hours: 0,
        minutes: 0,
        seconds: 0
      };
      if (input == "inactive" || typeof(input) === 'undefined') {
        ret.seconds = -1;
      } else if (input !== "" && input != "Expired") {
        input = Math.floor(input);
        ret.seconds = input % 60;
        ret.minutes = Math.floor(input / 60) % 60;
        ret.hours = Math.floor(input / 3600) % 24;
        ret.days = Math.floor(input / 86400);
      }
      return ret;
    }
    toString() {
        var self = this;
        var ret = "";
        if (self.state.time.days > 0) {
          ret += self.state.time.days + "d:";
        }
        if (self.state.time.hours > 0) {
          ret += self.state.time.hours + "h:";
        }
        if (self.state.time.minutes > 0) {
          ret += self.state.time.minutes + "m:";
        }
        if (self.state.time.seconds > 0) {
          ret += self.state.time.seconds + "s";
        }
        if (ret == "") {
          return "--";
        }
        if (self.state.time.seconds == 0 && ret != "") {
          ret = ret.substring(0, ret.length - 1);
        }
      return ret;
    }
    updateTime() {
      var self = this;
      var time = self.state.time;
      if (time.seconds < 0) {
        return;
      }
      if (time.seconds == 0 && time.minutes == 0 && time.hours == 0 && time.days == 0) {
        return;
      }
      if (time.seconds != 59) {
        self.setState({time:{
          seconds: time.seconds + 1,
          minutes: time.minutes,
          hours: time.hours,
          days: time.days
        }});
      } else if (time.minutes != 59) {
        self.setState({time:{
          seconds: 0,
          minutes: time.minutes + 1,
          hours: time.hours,
          days: time.days
        }});
      } else if (time.hours != 23) {
        self.setState({
          seconds: 0,
          minutes: 0,
          hours: time.hours + 1,
          days: time.days
        });
      } else {
        self.setState({
          seconds: 0,
          minutes: 0,
          hours: 0,
          days: time.days + 1
        });
      }
    }
    componentDidMount() {
      var self = this;
      if (self.state.run) {
        clearInterval(self.tick);
        self.tick = setInterval(self.updateTime, 1000);
      }
    }
    componentWillUnmount() {
      clearInterval(this.tick);
    }
    render() {
      var self = this;
      var html = (
        <span>
            {
              self.toString()
            }
        </span>
      );
      return html;
      }
    }
export default UpTime
