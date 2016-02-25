"use strict";
//Adjustable variables
var
  nRequests = 50, //Number of requests to make
  delayBetweenRequestsMs = 0, //Delay between requests
//Change below only if you know what you're doing
  scriptID = 'create_delete_continuous', //script id to run
  IMSI = 123456789012345, //IMSI
  templateName = "att", //Name of UE template
//DO NOT TOUCH -
  colonyName,
  serviceName;

//////////////////
//Test invocation
setUp().done(function(){
  testPoll(scriptID);
});
/////////////////

//Function declarations
function setUp(){
  return rw.api.json('/api/operational/colony/').done(function(data){
    console.log(data.colony[0].name, data.colony[0]["rw-appmgr:trafsim-service"][0].name, data)
    colonyName = data.colony[0].name;
    serviceName = data.colony[0]["rw-appmgr:trafsim-service"][0].name;
  });
}
function startScript(id){
  return rw.api.rpc('/api/operations/trafsim-start',{
    "input": {
      "colony": {
        "name": colonyName,
        "trafsim-service": {
          "name": serviceName,
          "execute-script": {
            "id": id,
            "ue-sim-name": templateName
          }
        }
      }
    }
  });
}
function stopScript(id){
  return rw.api.rpc('/api/operations/trafsim-stop',{
    "input": {
      "colony": {
        "name": colonyName,
        "trafsim-service": {
          "name": serviceName,
          "ue-sim": {
            "imsi": IMSI
          }
        }
      }
    }
  });
}
function testPoll(id){
  var i = 0;
  startScript(id).then(uestate);
  uestate();
  function uestate () {
    if(i<nRequests){
      setTimeout(function(){
        i++;
        return rw.api.json('/api/operational/ltesim/ue-state/' + IMSI + '?deep').then(uestate);
      }, delayBetweenRequestsMs);
    } else {
      done();
    }
  }
}




