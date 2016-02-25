    //scriptLogFn.$inject = ['$timeout', 'ueScript'];
(function (window, angular, React, rw, undefined) {


  angular.module('scriptLog',['ngResource'])
    .factory('scriptLog', function ($timeout) {
      var thePoll,
        stopLog = false,
        lastStamp,
        script_id = -1,
        pollingFn,
        theLogs = [],
        theTimeout,
        group_call_id,
        call_id,
        verbosity = 0;
        severity = 'debug';

      return {
        getCallID: function(){
          return {
            group_call_id: group_call_id,
            call_id: call_id
          }
        },
        getID: function() {return script_id},
        setID: function(id){script_id = id;},
        fetchLogs: function (id, timestamp) {
          console.log(timestamp);
          var input = timestamp ? { "start-time": timestamp } : { "all": "" };
          //console.info ("Going In: " + timestamp);
          input.verbosity = verbosity;
          //console.log(verbosity);
          input.filter = {
            'groupcallid': id,

            'category': {
              'name': 'rw-appmgr-log',
              'severity' : severity
            }

          };

          //  input.count = 2;
          //return{
          //  done:function(c){
          //
          //  }
          //}
          return rw.api.rpc('/api/operations/show-logs',{ "input": input });
        },
        pollLogs: function (id,d, callback) {
          if (stopLog) {
            console.log('stopLog == true');
            return;
          }
          var self = this,
            stamp;
          if(d) {
            stamp = d.output["log-summary"][0]["trailing-timestamp"];//getTimeStamp(theLogs.slice(-1)[0].msg);
          }
          //console.log(d);
          //console.log(theLogs);
          if (d && d.output.logs) {
            if(d.output.logs) theLogs = theLogs.concat(d.output.logs);
            var len = d.output.logs.length;
            var group, call;
            for(var i = len - 1; 0 < i; i--){
              try{
                group = d.output.logs[i].msg.match(/groupcallid:(\d+)/)[1];

              } catch (e){
                group = false;
              }
              try{
                call = d.output.logs[i].msg.match(/\scallid:(\d+)/)[1];

              } catch (e){
                call = false;
              }
              if(group && call){
                group_call_id = group;
                call_id = call;
                break;
              }
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
        },
        setSeverity: function(input) {
          severity = input;
        }
      };

      function getTimeStamp(str) {
        return str.match(/\S*/)[0];
      }
    })})(window, window.angular, window.React, window.rw);
