rw.JsBin = function(log, done) {
  this.log = log;
  this.done = done;
  // divert calls to this
  rw.api.handleAjaxError = this.errorHandler('ajax');
  return this;
}

rw.JsBin.prototype = {

  errorHandler : function(msg) {
    var self = this;
    return function(req, status, err) {
      self.log('**ERROR ' + msg + ' ' + req.responseText);
    }
  },

  bg : function(job, jobs) {
    return jobs ? jobs.then(job) : job();
  }
};

