var DBNAME = 'tragenbig';

angular.module('offlineData',[])
  .service('offlineData',function() {
    var service = {};
    service.vnf = getOfflineDB(DBNAME,'/vnf/')
    service.trafsim = {
      getConfig : getOfflineDB(DBNAME,'/trafsim/config')
    };
    service.trafgen = {
      start: function(){
        console.log('fake starting');
      },
      stop: function(){
        console.log('fake stopping');
      }
    }
    return service;
  })





//FUNCTIONS FOR OFFLINEDATA

/*****
Retrieve prepopulated data from IndexedDB.
Version is currently hardcoded.
TODO add upgrade handler.
*****/
function getOfflineDB(dbname,key){
  var zedata;
  var getConfig = new Promise(function (resolve, reject) {
    var request = window.indexedDB.open(dbname, 2);
    request.onsuccess = function (event) {
      var db = event.target.result;
      var query = db.transaction('offline').objectStore('offline').get(key)
      query.onsuccess = function (e) {
        zedata = e.target.result.data;
        resolve(zedata)
      }
      query.onerror = function (e) {
        console.log('error')
      }
    }
  })
  return getConfig;
}