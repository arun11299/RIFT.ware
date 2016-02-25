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

      var countersUrl = baseUrl + '?select=statistics/service/counters';
      jobs.push(rw.api.get(countersUrl, accept));

      var commandsUrl = baseUrl + '?select=statistics/service/counters/protocol/commands';
      jobs.push(rw.api.get(commandsUrl, accept));

      var timersUrl = baseUrl + '?select=statistics/service/counters/timer';
      jobs.push(rw.api.get(timersUrl, accept));
    });
  });

  return $.when.apply($, jobs);
};