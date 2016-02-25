//scriptLogFn.$inject = ['$timeout', 'ueScript'];
(function (window, angular, React, rw, undefined) {


angular.module('scriptLog',['ngResource'])
  .factory('scriptLog', function ($timeout) {
  var thePoll,
    stopLog = false,
    lastStamp,
    script_id,
    pollingFn,
    theLogs = [],
    theTimeout,
    group_call_id,
    call_id,
    verbosity = 0;

  return {
    getCallID: function(){
      return {
        group_call_id: group_call_id,
        call_id: call_id
      }
    },
    setID: function(id){script_id = id;},
    fetchLogs: function (id, timestamp) {
      var input = timestamp ? { "start-time": timestamp } : { "all": "" };
        //console.info ("Going In: " + timestamp);
      input.verbosity = verbosity;
      //console.log(verbosity);
      input.filter = {
        'groupcallid': id,

        'category': {
          'name': 'rw-appmgr-log'
        }
      };

     return rw.api.rpc('/api/operations/show-logs',{ "input": input });
    },
    pollLogs: function (id,d, callback) {
      if (stopLog) {
        return;
      }
      var self = this,
        stamp;
      if(d)stamp = d.output["trailing-timestamp"];//getTimeStamp(theLogs.slice(-1)[0].msg);
      if (d) {
        if(d.output.logs) theLogs = theLogs.concat(d.output.logs);
        var len = d.output.logs;
        try{
          group_call_id = d.output.logs[len-1].match(/groupcallid:(\d+)/)[1];
          call_id = d.output.logs[len-1].match(/\scallid:(\d+)/.)[1];
        } catch (e){
          console.log('Regex failed', e);
        }
        if (callback !== 'undefined') {
          callback(theLogs);
        }
      }
      if(script_id == id) {
        theTimeout = $timeout(function () {
          self.fetchLogs(id, stamp).done(function (data) {
            self.pollLogs(id, data, callback);
          });
        }, 1000);
      }
    },
    stopTimeout: function(){
      $timeout.cancel(theTimeout);
    },
    showLogs: function () {
      return theLogs;
    },
    clearLogs: function() {
      theLogs = [];
    },
    setStopLog: function(state){
      stopLog = state;
    },
    stopLog: function (callback) {
      console.log('stopping log', pollingFn)
      //$timeout.cancel(pollingFn);
      stopLog = true;
      if(callback)callback();
    },
    startLog: function (id,callback) {
      stopLog = false;
      this.pollLogs(id,undefined, callback);

    },
    setVerbosity: function (input) {
      verbosity = input;
    }
  };

  function getTimeStamp(str) {
    return str.match(/\S*/)[0];
  }
})})(window, window.angular, window.React, window.rw);
