var utilityFn = require('./jsonParserFn.js');

module.exports = (function () {

function jsonParser(schema, fnLibrary) {

  //if no function library object is specified, the default one is used.
  fnLibrary = fnLibrary || utilityFn;

  var generateData = generatedData;
  var returnedObject = {}

  for(k in schema) {
    returnedObject[k] = generateData(schema[k])
  }
  return returnedObject;

  function generatedData (obj) {
    //for (k in obj) {
      //verify own property
      //check if value is special repeater type
      var isRepeat = (obj.constructor === Array &&
      (obj[0].indexOf('repeat(') > -1))

      if (isRepeat) {
        //Create a new array to store each of the repeated entries;
        var newObj = [];
        //Get the min and max number of repeats
        var Rn = obj[0].match(/repeat\((\d+),?(\d+)?\)/);
        var count = repeatCount(parseInt(Rn[1]), parseInt(Rn[2]));
        var objFn = obj[1];
        //For each repeat create an object
        for (var i = 0; i < count; i++) {
          var tempObj = {};
          // For each definition in the object
          for (j in objFn) {
            if (objFn.hasOwnProperty(j)) {
              var value = objFn[j];
              var isRepeater = (value.constructor === Array &&
              (value[0].indexOf('repeat(') > -1));
              if (isRepeater) {
                var someObj = {}
                someObj[j]
                tempObj[j] = generateData(value);
              } else {
                tempObj[j] = parseFn(value);
              }

            }
          }
          newObj.push(tempObj);
        }
      } else {
        newObj = obj;

      }
    return newObj;
  };

  function repeatCount(min, max) {
    if (!max) {
      return min;
    }
    return Math.floor(Math.random() * (max - min + 1)) + min;
  }

  function parseFn(str) {
    if (str.constructor === Array || str.constructor === Object) {
      return str;
    } else {
      var fnStr = str.match(/\{\{(\w+)\(([^)]+)?\)\}\}/);
      if (fnStr) {
        var fnName = fnStr[1];
        var fnArgs = fnStr[2];
        if (fnArgs) {
          fnArgs = fnArgs.split(/\s*,\s*/);
        }
        return fnLibrary[fnName].apply(fnLibrary, fnArgs);
      } else {
        return str;
      }
    }


  }

}


  return {
    jsonParser: jsonParser
  }

})();


