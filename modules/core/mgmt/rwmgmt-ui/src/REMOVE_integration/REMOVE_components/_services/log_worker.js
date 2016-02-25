"use strict";

var api_server = 'http://localhost:5050';
var script_id;
var logs;
var req = new XMLHttpRequest();
var state = false;


onmessage = function(e){
  if(e.data[0]){
    var data = e.data[0];
    if(data.script_id){
      script_id = data.script_id;
    }
    if(data.state == 'start'){
      postMessage(['started']);
      state = true;
      fetchLogs();
    }
    if(data.state == 'stop'){
      postMessage(['stopped']);
      state = false;
      //stopLogs();
    }
  }
};

function startLogs(){
  fetchLogs();
}

function fetchLogs(timestamp){
  //console.log(req)
  if(state) {
    req.onload = callback.bind(req);
    var input = timestamp ? {"start-time": timestamp} : {"all": ""};
    //input.verbosity = verbosity;
    input.filter = {
      'groupcallid': 1,
      'category': {
        'name': 'rw-appmgr-log'
      }
    };
    req.open("POST", api_server + "/api/operations/show-logs");
    req.setRequestHeader("Content-Type", "application\/vnd.yang.data+json");
    req.setRequestHeader("Accept", "application\/vnd.yang.data+json");
    req.send(JSON.stringify({"input": input}));
  }
}
var theLogs = [];
function pollLogs(d) {
  console.log(d);
  var stamp;
  if(d) stamp = d.output["trailing-timestamp"];//getTimeStamp(theLogs.slice(-1)[0].msg);

  setTimeout(function () {
      fetchLogs(stamp);
    }, 2000);
}

function callback (){
  var  data = JSON.parse(this.responseText);
  logs = data.output.msg;
  postMessage(JSON.stringify(logs));
  pollLogs(data)
};