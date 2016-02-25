
// dummies websockets so call calls to websockets silently do nothing.
// NOTE:
//  For normal use this in your test's "attached" method as the 
//  document.websocket method does not exist until AFTER document
//  is ready
function stubWebsocket() {
  console.log('stubbing ws');
  var dummySocket = {
    on : sinon.spy(),
    emit: sinon.spy()
  };
  sinon.stub(document, "websocket").returns(dummySocket);
}

// dummies so elements do not attempt to communicate in constructor
// replace with mocks in test bodies
// NOTE:
//  For normal use this in your test's "ready" method
function stubAjax() {
  console.log('stubbed ajax');
  sinon.stub(jQuery, "ajax");
  sinon.stub(jQuery, "getJSON");
}

// ajax is useful to load test files
function restoreAjax() {
  console.log('restoring ajax');
  jQuery.ajax.restore();
  jQuery.getJSON.restore();
}

// synchronously get json
function getTestJSON(url) {
  return getTestFile(url, 'json');
}

// synchronously get text
function getTestFile(url, dataType) {
  var data = null;
  response = jQuery.ajax({
    type: 'GET',
    url : url,
    async: false,
    dataType: dataType,
    success: function(response) {
      data = response;
    },
    error: function(request, status, err) {
      console.log('failed to load url ' + url, request, status, err);
    }
  });
  return data;
}

// defer synchronously get text
function getTestFileDeferred(url, dataType) {
  return function() {
    return $.Deferred(function(defer) {
      var dom = getTestFile(url, dataType);
      defer.resolve(dom);
    }).promise();
  };
}

function htmlTestRunner(runner) {
  if (window.top !== window) {
    runner.on("end", function() {
      parent.postMessage('ok', '*');
    });
    runner.on("fail", function(x) {
      parent.postMessage({error: x.err.message}, '*');
    });
  }
}