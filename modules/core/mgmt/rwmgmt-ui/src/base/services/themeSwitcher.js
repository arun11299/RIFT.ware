// Currently has jQuery dependency

(function(window) {
  var themeTag;
  var themeLoader = {};
  
  themeLoader.init = init;
  themeLoader.load = loadTheme.bind(this);

  if (typeof(window.$rw) == 'undefined') {
    window.$rw = {}
  }

  window.$rw.theme = themeLoader;
  // Function definitions

  function init() {
    getThemeList().done(function(data) {
      themeLoader.list = data;
      loadTheme();
    })
  }

  function getThemeList() {
    var themes;
    var promise = jQuery.Deferred();
    if (typeof(window.$rw.definitions) == 'undefined') {
      $.get('offline/definitions.json').done(function(data) {
        promise.resolve(data.themes);
      })
    } else {
      promise.resolve(window.$rw.definitions.themes);
    }
    return promise;
  }

  function loadTheme(theme) {

    var themeTag;
    var currentTheme =  (typeof(theme) == 'undefined') ? themeLoader.list.default : theme;

    if (typeof(window.theme) != 'undefined') {
      themeTag = window.theme;
      themeTag.href = currentTheme;
    } else {
      themeTag = document.createElement('link');
      themeTag.media = 'all';
      themeTag.rel = 'stylesheet';
      themeTag.id = 'theme';
      themeTag.href = currentTheme;
      document.head.appendChild(themeTag);
    }

  return themeLoader.currentTheme = currentTheme;
  }


  if (typeof module === 'object') {
      module.exports = themeLoader;
  }


})(window);
