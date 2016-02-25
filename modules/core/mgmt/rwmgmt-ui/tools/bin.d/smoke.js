// APIs that are fundamental to UI

var logFilter = { input: { all : "" } };

var jobs = [
  rw.api.json('/fpath'),
  rw.api.json('/vcs'),
  rw.api.json('/vnf'),
  rw.api.json('/api/operational/colony'),
  rw.api.rpc('/api/operations/show-logs', logFilter, jsbin.errorHandler)
];
$.when.apply($, jobs).always(done);

