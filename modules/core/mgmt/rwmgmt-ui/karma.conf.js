// Karma configuration
// Generated on Sun Feb 01 2015 15:39:24 GMT-0500 (EST)
module.exports = function (config) {
  config.set({
    frameworks: ['browserify', 'jasmine'],
    // list of files / patterns to load in the browser
    files: [
      './node_modules/es5-shim/es5-shim.min.js',
      './lib/angular/angular.min.js',
      './lib/scriptjs/dist/script.min.js',
      './node_modules/angular-mocks/angular-mocks.js',
      './lib/jquery/dist/jquery.min.js',
      './node_modules/jasmine-jquery/lib/jasmine-jquery.js',
      './lib/react/react-with-addons.min.js',
      'build/core/**/*.js',
      'build/core/*.js',
      './test/spec/*.js',
      'build/modules/views/*.html',
      // {pattern: 'build/modules/views/*.html', watch: true, served: true, included: false},
      {pattern: 'build/offline/*.json', watch: true, served: true, included: false}
    ],

    basePath:'build/',

    browserify: {
      watch: true,
      debug: true
    },
    preprocessors: {
      'test/spec/*.js': ['browserify'],
      'build/modules/views/*.html': ['ng-html2js']
    },
    ngHtml2JsPreprocessor: {
      stripPrefix: 'build/',
      moduleName: 'templates'
    },
    // test results reporter to use
    // possible values: 'dots', 'progress'
    // available reporters: https://npmjs.org/browse/keyword/karma-reporter
    reporters: ['progress'],
    loggers: [{type: 'console'}],
    // web server port
    port: 9876,
    // enable / disable colors in the output (reporters and logs)
    colors: true,
    // level of logging
    // possible values: config.LOG_DISABLE || config.LOG_ERROR || config.LOG_WARN || config.LOG_INFO || config.LOG_DEBUG
    logLevel: config.LOG_INFO,
    // enable / disable watching file and executing tests whenever any file changes
    autoWatch: true,
    // start these browsers
    // available browser launchers: https://npmjs.org/browse/keyword/karma-launcher
    browsers: ['PhantomJS'],
    // Continuous Integration mode
    // if true, Karma captures browsers, runs the tests and exits
    singleRun: false
  });
};

