//TODO Most watchers currently run a generalized function that copies over all files. Should go through copy functions and optimize.
console.time('Loading plugins')

var
  argv = require('yargs').argv,
  gulp = require('gulp'),
  browserify = require('browserify'),
  changed = require('gulp-changed'),
  connect = require('gulp-connect'),
  concat = require('gulp-concat'),
  del = require('del'),
  doxx = require('gulp-doxx'),
  es = require('event-stream'),
  fs = require('fs'),
  inject = require('gulp-inject'),
  karma = require('karma'),
  path = require('path'),
  less = require('gulp-less'),
  plumber = require('gulp-plumber'),
  react = require('gulp-react'),
  reactify = require('reactify'),
  rename = require('gulp-rename'),
  requireDir = require('require-dir'),
  source = require("vinyl-source-stream"),
  svgstore = require('gulp-svgstore'),
  sourcemaps = require('gulp-sourcemaps'),
  uglify = require('gulp-uglify'),
  watchify = require('watchify');

requireDir('./tasks');

console.timeEnd('Loading plugins')
var
  buildDirectory = argv.making ? argv.making : 'build',
  coreComponentsPath = './src/base/components/',
  modulesPath = './src/modules/';


/**
 * Primary Tasks
 */
//BACKUP
// gulp.task('copy', ['copy-integration-html', 'copy-integration-data', 'copy-lib',
//   'copy-offline-files', 'copy-ui2-files']);
// gulp.task('watch-copy', ['watch-integration-html', 'watch-integration-data',
//   'watch-offline-files', 'watch-ui2-files']);
// gulp.task('build', ['less', 'concat-lib', 'copy','browserify-components',
//   'browserify','build-modules']);

gulp.task('copy', ['copy-lib',
  'copy-offline-files', 'copy-ui2-files', 'fonts','copy-integration-data']);
gulp.task('watch-copy', ['watch-offline-files', 'watch-ui2-files']);
gulp.task('build', ['less', 'concat-lib', 'copy','browserify-components',
  'browserify','browserify-modules-build','svg']);

// Starts Karma test runner
gulp.task('test', ['copy','build-modules'], function () {
  return karma.server.start({
    basePath: __dirname,
    configFile: __dirname + '/karma.conf.js'
  }, function () {
  });
});

// Generates documentation for UI
gulp.task('generate-documentation', genDocs);

/**
 * Development Tasks
 */

gulp.task('dev_components', ['watch-components']);
gulp.task('dev', ['watch-copy', 'watch-browserify', 'watch-module-views', 'watch-modules', 'test', 'watch-less', 'concat-lib', 'browserify-modules-dev', 'webserver', 'svg','dev_components','copy-integration-data']);
gulp.task('dev_svg', function(){
  return injectSVG(argv.folder, argv.file);
});

/**
 * Watch Tasks
 * Used for Development
 */

// Browserify base files.
// app.js, components/*.js
gulp.task('watch-browserify', ['browserify'], function() {
  return gulp.watch([
    'src/*.js',
    './src/**/**/*.js',
    '!./src/base/services/jsonParser.js',
    '!./src/base/services/jsonParserFn.js',
    '!src/base/components/**/*.js'
  ], browserifyShare.bind(null, true));
});

// Browserify only components
gulp.task('watch-components', ['browserify-components'], function() {
  return gulp.watch([
    './src/base/services/jsonParser.js',
    './src/base/services/jsonParserFn.js',
    'src/base/components/**/*.js'
  ], browserifyComponents.bind(null, true));
});



/**
 * General Tasks
 */

// Browserify base files
gulp.task('browserify', ['copy'], function(){
  return browserifyShare()
} );

// Browserify components only
gulp.task('browserify-components', function() {
  return browserifyComponents();
});

gulp.task('browserify-modules-dev', ['copy-module-views','watch-module-css','watch-module-less',], function() {
  return setupModules('rift', true);
})

gulp.task('browserify-modules-build', ['copy-module-views','copy-module-less','copy-module-css'], function() {
  return setupModules('rift', false);
})


//Remove prexisting compiled files
gulp.task('clean', function() {
  del(buildDirectory);

  del('docs/');
});





// Concatenates and Injects SVG into build/index.html
// Requires that 'copy' task has completed first.
gulp.task('svg',['copy'], function () {
  // injectSVG('build', 'new_integration.html');
  //return injectSVG('build', 'index.html');
  return injectSVG(buildDirectory + '/', 'index.html');
});


gulp.task('svg-test', function() {
  return injectSVG('test/components', '*.html');
});

// Start a webserver
gulp.task('webserver', function () {
  connect.server({
    root: 'build/'
  });
});


/**
 * Functions
 */


function injectSVG(folder, file, svgs) {
  var svgs = gulp
    .src('src/base/icons/*.svg')
    .pipe(svgstore({ inlineSvg: true }));

  return gulp
    .src(folder + '/' + file)
    .pipe(inject(svgs, { transform: fileContents}))
    .pipe(gulp.dest(folder));

  function fileContents (filePath, file) {
    return file.contents.toString();
  }
}

//This should be refactored with browserifyShare ()
function browserifyComponents(shouldWatch) {
  var b = browserify({
    cache: {},
    packageCache: {},
    fullPaths: true,
    debug: true
  });
  if (shouldWatch) {
    b = watchify(b);
    b.on('update', function () {
      console.log('Updating components')
      return bundleShare(b);
    });
  }
  //Add file to browserify list
  b.add('./src/base/components/components.js');
  b.add('./src/base/components/uiHelper.js');
  //b.add('./src/base/services/jsonPa)rser.js')
  return bundleShare(b);
  function bundleShare(b) {
    return b.bundle()
      .pipe(source('components.js'))
      .pipe(gulp.dest('./test/components/js'));
  }
}

function browserifyModules(shouldWatch, type, folder) {
  var b = browserify({
    cache: {},
    packageCache: {},
    fullPaths: true,
    debug: true
  });
  if (shouldWatch) {
    b = watchify(b);
    b.on('update', function () {
      console.log('Updating modules')
      return bundleShare(b);
    });
  }
  b.transform(reactify);
  //Add file to browserify list
  b.add('./src/modules/' + type + '/' + folder + '/'+ folder + '.mod.js');
  //b.add('./src/base/services/jsonParser.js')
  return bundleShare(b);
  function bundleShare(b) {
    return b.bundle()
      .pipe(source(folder + '.browserify.js'))
      .pipe(gulp.dest(buildDirectory + '/modules/' + type));
  }
}

function browserifyShare(shouldWatch) {
  var b = browserify({
    cache: {},
    packageCache: {},
    fullPaths: true,
    debug: true
  });
  if (shouldWatch) {
    b = watchify(b);
    b.on('update', function () {
      console.log('Updating all else');
      return bundleShare(b);
    });
  }
  //Add file to browserify list
  b.add('./src/app.js');
  b.add('./src/base/components/components.js')
  return bundleShare(b);
  function bundleShare(b) {
    return b.bundle()
      .pipe(source('main.js'))
      .pipe(gulp.dest(buildDirectory + '/'));
  }
}


function concatComponents() {
  var path = coreComponentsPath;
  return gulp.src(path + '/**/*.js')
    .pipe(sourcemaps.init())
    .pipe(concat('components.js'))
    //.pipe(uglify())
    .pipe(sourcemaps.write('../maps'))
    .pipe(gulp.dest(buildDirectory + '/base'));
}


function genDocs() {
  return gulp.src([
    './src/blank.js',
    './src/base/services/module_loader.js',
    './src/base/**/**/*.js',
    './src/modules/**/*.js'
  ])
    .pipe(doxx({title: 'UI Framework', urlPrefix: '/rwmgmt-ui/docs'}))
    .pipe(gulp.dest('docs'));
}

function getFolders(dir) {
  var folders = [];
  var top = fs.readdirSync(dir)
    .filter(function (file) {
      return fs.statSync(path.join(dir, file)).isDirectory();
    });
  for (var i = 0; i < top.length; i++) {
    var temp = fs.readdirSync(path.join(dir, top[i])).filter(function (file) {
      return fs.statSync(path.join(dir, top[i], file)).isDirectory();
    });
    temp = temp.map(function (item) {
      return top[i] + '/' + item;
    });
    folders = folders.concat(temp);
  }
  return folders;
}
function getFolderOne(dir) {
  return fs.readdirSync(dir)
    .filter(function (file) {
      return fs.statSync(path.join(dir, file)).isDirectory();
    });
}

function setupModules(type, shouldWatch) {
  var ModulePath = './src/modules/' + type;
  var folders = getFolderOne(ModulePath);
  var streams = folders.map(function (folder) {
    return browserifyModules(shouldWatch, type, folder)
  });
  return es.concat.apply(null, streams);
}

// gulp.task('react-components', function() {
//   return gulp.src('./src/base/components/*.jsx')
//   .pipe(react())
//   .pipe(gulp.dest('./'));
// })
