// Config
var logFilter = {
  input: {all : ""}
};

var nRequests = 5;
var delayBetweenRequestsMs = 100;

// Exec
callLog(log, logFilter, delayBetweenRequestsMs, nRequests);

// Functions
function lastTimestamp(data) {
  if (typeof(data) == 'undefined' || !('output' in data)) {
    return null;
  }
  if ('trailing-timestamp' in data.output) {
    return data.output['trailing-timestamp'];
  }
  if (!('logs' in data.output) || data.output.logs.length == 0) {
    return null;
  }
  var lastLine = data.output.logs.slice(-1)[0].msg;
  var lastTimestamp = lastLine.match(/\S*/)[0];
  return lastTimestamp;
}

function callLog(logger, logFilter, delayBetweenRequestsMs, nRequests, nRemaining) {
  var start = new Date().getTime();
  var url = '/api/operations/show-logs';
  var n = typeof(nRemaining) == 'undefined' ? nRequests : nRemaining;
  logger('#' + (1 + nRequests - n) + '. ' + url + ' - ' + JSON.stringify(logFilter) + '...');
  var err = function(e) {
    var end = new Date().getTime();
    var time = end - start;
    logger('' + time + 'ms - ' + e.status + ' - ' + e.statusText);
  };
  rw.api.rpc(url, logFilter, err).done(function(data) {
    var end = new Date().getTime();
    var time = end - start;
    var hdr = 'rest:time-spent:' + time + 'ms, ';
    if ('output' in data) {
      for (var prop in data.output) {
        if (prop != 'logs') {
          hdr += prop + ':' + data.output[prop] + ', ';
        }
      }
    }

    if ('logs' in data.output) {
      logger(hdr + data.output.logs.length + ' logs');
    } else {
      logger(hdr + 'no logs');
    }

    var ts = lastTimestamp(data);
    if (ts) {
      delete logFilter.input.all;
      logFilter.input['start-time'] = ts;
    }

    if (n > 1) {
      setTimeout(function() {
        callLog(logger, logFilter, delayBetweenRequestsMs, nRequests, n - 1);
      }, delayBetweenRequestsMs);
    } else {
      done();
    }
  });
};

