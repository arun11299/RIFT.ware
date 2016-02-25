// Calls for Diameter
var accept = 'application/vnd.yang.collection+json';

rw.api.get('/api/running/colony?select=trafsim-service/name', accept).then(function (trafSims) {
  loadStats(trafSims).always(done);
});

function loadStats(trafSims) {
  var jobs = [];
  _.each(trafSims.colony, function(colony) {
    var colonyId = colony.name;
    _.each(colony['rw-appmgr:trafsim-service'], function (trafsim) {
      var baseUrl = '/api/operational/colony/' + colonyId + '/trafsim-service/' + trafsim.name;
      var countersUrl = baseUrl + '/statistics/service/counters';
      jobs.push(rw.api.get(countersUrl, accept));
    });
  });

  return $.when.apply($, jobs);
};