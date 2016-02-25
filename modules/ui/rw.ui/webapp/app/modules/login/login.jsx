/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
import React from 'react/addons';
import Utils from '../utils/utils.js';
import './login.scss'
class LoginScreen extends React.Component{
  constructor(props) {
    super(props);
    var API_SERVER =  rw.getSearchParams(window.location).api_server;
    if (!API_SERVER) {
      window.location.href = "//" + window.location.host + '/index.html?api_server=http://localhost';
    }
    this.state = {
      username: '',
      password: ''
    };

  }
  updateValue = (e) => {
    let state = {};
    state[e.target.name] = e.target.value;
    this.setState(state);
  }
  validate = () => {
    let state = this.state;
    console.log(this.state)
    if (state.username == '' || state.password == '') {
      console.log('false');
      return false;
    } else {
      Utils.setAuthentication(state.username, state.password, function() {
         let hash = window.sessionStorage.getItem("locationRefHash") || '#/';
        if (hash == '#/login') {
          hash = '#/'
        }
        window.location.hash = hash;
      });

    }
  }
  render() {
    let html;
    html = (
      <div className="login-cntnr">
        <div className="logo"> </div>
        <h1 className="riftio">RIFT.IO</h1>
        <p>
            <input type="text" placeholder="Username" name="username" value={this.state.username} onChange={this.updateValue} ></input>
        </p>
        <p>
            <input type="password" placeholder="Password" name="password" onChange={this.updateValue} value={this.state.password}></input>
        </p>
        <p>
           <a className="sign-in" onClick={this.validate} style={{cursor: 'pointer'}} type="button">Sign In</a>
        </p>
      </div>
    )
    return html;
  }
}

export default LoginScreen;
