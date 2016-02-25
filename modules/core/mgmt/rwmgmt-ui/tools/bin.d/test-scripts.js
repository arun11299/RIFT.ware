rw.api.get('/api/operational/ltesim/test-script').then(function(data) {
  if (typeof(data) == 'undefined') {
    log('no data!');
  } else {
    log('got data');
  }
});
