var
  argv = require('yargs').argv,
  concat = require('gulp-concat'),
  gulp = require('gulp'),
  plumber = require('gulp-plumber'),
  rename = require('gulp-rename'),
  uglify = require('gulp-uglify');

var buildDirectory = argv.making ? argv.making : './build';
var plumberConfig = {
  errorHandler: function(error) {
    console.log(error.toString())
    this.emit('end');
  }
};
//Copy integration files from root+src to build folder
gulp.task('copy-integration-html', function(){
  copyIntegrationHTML();
});

gulp.task('copy-new-integration-html', copyTestHTML);

gulp.task('watch-integration-html', function(){
  return gulp.watch(['./integration.html','./components.html', './config.html'], copyIntegrationHTML);
});


// Copy and Watch files in the integration folder
gulp.task('copy-integration-data', function(){
  copyIntegrationFiles();
});
gulp.task('watch-integration-data', ['copy-integration-data'], function(){
  return gulp.watch('./src/integration/**/*', copyIntegrationFiles);
});


//Copy lib files to build  - necessary for integration
gulp.task('copy-lib', function(){
  //TODO: NEEDS TO MOVE
  gulp.src('./src/integration/ridgets/js/rw.js')
    .pipe(gulp.dest(buildDirectory + '/integration/ridgets/js/'));
  gulp.src('./src/integration/img/*.*')
    .pipe(gulp.dest(buildDirectory + '/integration/img/'));


  gulp.src('./src/integration/_data/_css/style.css')
    .pipe(gulp.dest(buildDirectory + '/integration/_data/_css/'));
  gulp.src('./src/integration/ridgets/css/page.css')
    .pipe(gulp.dest(buildDirectory + '/integration/ridgets/css/'))

  return gulp.src('./lib/**/*')
    .pipe(plumber(plumberConfig))
    .pipe(gulp.dest(buildDirectory + '/lib'));
});

// Copy and Watch offline json files
gulp.task('copy-offline-files', function(){
  copyOfflineFiles();
});
gulp.task('watch-offline-files', ['copy-offline-files'], function(){
  return gulp.watch('./src/base/offline/*', copyOfflineFiles);
});

// Copy and Watch UI-2 Index.html
gulp.task('copy-ui2-files', function(){
  copyUITwoFile();
});
gulp.task('watch-ui2-files', ['copy-ui2-files'], function(){
  return gulp.watch('./src/ui2.html', copyUITwoFile);
});

//Concats library files for ui2
gulp.task('concat-lib', function () {
  return gulp.src([
                  './node_modules/highlight.js/lib/highlight.js',

    './lib/jquery/dist/jquery.min.js',
    './lib/angular/angular.min.js',
    './lib/angular-resource/angular-resource.min.js',
    './lib/angular-ui-router/release/angular-ui-router.min.js',
    './lib/oclazyload/dist/ocLazyLoad.min.js',
    'lib/angular-mocks/angular-mocks.js',
    './lib/angular-ui-bootstrap-bower/ui-bootstrap.min.js',
    './lib/angular-ui-bootstrap-bower/ui-bootstrap-tpls.min.js',
    './lib/codemirror/lib/codemirror.js',
    './lib/angular-ui-codemirror/ui-codemirror.min.js',
    './lib/scriptjs/dist/script.min.js',

    './lib/platform/platform.js',
    'lib/ng-websocket/ng-websocket.js',
    'lib/mock-socket/dist/mock-socket.min.js',
    'lib/bower-jvectormap/jquery-jvectormap-1.2.2.min.js',
    'lib/bower-jvectormap/jquery-jvectormap-world-mill-en.js',
    'lib/numeral/numeral.js',
    'lib/d3/d3.min.js',
    'lib/jsonpath/lib/jsonpath.js',
    'lib/underscore/underscore.js',
    'lib/socket.io-client/dist/socket.io.js',
    'lib/nouislider/jquery.nouislider.min.js'
  ])
    .pipe(plumber(plumberConfig))
    .pipe(concat('vendor.js'))
    // .pipe(uglify())
    .pipe(gulp.dest(buildDirectory + '/'));
});

function copyTestHTML(){
return  gulp.src('./new_integration.html')
    .pipe(gulp.dest(buildDirectory + '/'))
}

// function copyIntegrationHTML(){
//   gulp.src('./integration.html')
//     .pipe(rename('index.html'))
//     .pipe(gulp.dest(buildDirectory + '/'));
//   gulp.src('./config.html')
//     .pipe(gulp.dest(buildDirectory + '/'));
//   return gulp.src('./components.html')
//     .pipe(gulp.dest(buildDirectory + '/integration'));
// }

function copyIntegrationHTML(){
  gulp.src('./index.html')
    .pipe(rename('index.html'))
    .pipe(gulp.dest(buildDirectory + '/'));
  gulp.src('./config.html')
    .pipe(gulp.dest(buildDirectory + '/'));
  return gulp.src('./components.html')
    .pipe(gulp.dest(buildDirectory + '/integration'));
}

function copyIntegrationFiles(){
  return gulp.src('./src/integration/**/*')
    .pipe(gulp.dest(buildDirectory + '/integration'));
}

function copyOfflineFiles(){
  return gulp.src('./src/base/offline/*')
    .pipe(gulp.dest(buildDirectory + '/offline'));
}

function copyUITwoFile(){
  return gulp.src(['./src/ui2.html','./index.html']).pipe(gulp.dest(buildDirectory + ''))
}
