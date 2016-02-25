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
  minifyCss = require('gulp-minify-css'),
  path = require('path'),
  less = require('gulp-less'),
  plumber = require('gulp-plumber'),
  rename = require('gulp-rename'),
  requireDir = require('require-dir'),
  source = require("vinyl-source-stream"),
  svgstore = require('gulp-svgstore'),
  sourcemaps = require('gulp-sourcemaps'),
  // transform = require('vinyl-transform'),
  uglify = require('gulp-uglify'),
  watchify = require('watchify');

var
  buildDirectory = argv.making ? argv.making : 'build',
  modulesPath = './src/modules/';
  lessConfig = {
    paths: [path.join(__dirname, 'src', 'base', 'less')]
  };
  plumberConfig = {
    errorHandler: function(error) {
      console.log(error.toString())
      this.emit('end');
    }
  };

gulp.task('build-modules', ['concat-module-js', 'copy-module-views','copy-module-less','copy-module-css']);

gulp.task('watch-modules',['watch-module-js','watch-module-css','watch-module-less','watch-module-views']);
gulp.task('watch-module-js', ['concat-module-js'], function() {
   gulp.watch(modulesPath + '/**/*.js', function() {
     concatModules(modulesPath, buildDirectory);
   });
});

gulp.task('watch-module-css', ['copy-module-css'], function() {
  return gulp.watch(modulesPath + '/**/*.css',  function() {
    copyModuleCSS(modulesPath, buildDirectory);
  });
});

gulp.task('watch-module-less', ['copy-module-less'], function() {
  return gulp.watch([modulesPath + '/**/*.less','./src/core.less', './src/base/less/*.less', './src/integration/**/*.less'], ['copy-module-less'])

});



gulp.task('watch-module-views', ['copy-module-views'], function() {
  return gulp.watch(modulesPath + '/**/*.html',  function() {
    copyModuleViews(modulesPath, buildDirectory);
  });
});


//Concatenate all scripts - currently only modules and core components
gulp.task('concat-module-js', function() {
  return concatModules(modulesPath, buildDirectory);
});

gulp.task('copy-module-css', function() {
  return copyModuleCSS(modulesPath, buildDirectory);
});

gulp.task('copy-module-less', function() {
  return copyModuleLESS(modulesPath, buildDirectory);
});

//Concatenate all scripts - currently only modules and core components
gulp.task('copy-module-views', function() {
  return copyModuleViews(modulesPath, buildDirectory);
});




function concatModules(mPath, bPath) {

  var folders = getFolders(mPath);
  var streams = folders.map(function (folder) {
  // var browserified = transform(function(filename) {
  //   var b = browserify(filename);
  //   return b.bundle();
  // });
    // b.add(bPath + '/modules' + folder + '.min.js');
    return gulp.src([
      path.join(mPath, folder, '/index.js'),
      path.join(mPath, folder, '/**/*.js')])
      // .pipe(browserified)
      .pipe(concat(folder + '.js'))
      .pipe(rename(folder + '.min.js'))

      .pipe(gulp.dest(bPath + '/modules'))


  });
  return es.concat.apply(null, streams);
}

function copyModuleViews(mPath, bPath) {
  var folders = getFolders(mPath);
  var streams = folders.map(function (folder) {
    return gulp.src([
      path.join(mPath, folder, '/views/*.html')])
      .pipe(gulp.dest(bPath + '/modules/views'));
  });
  return es.concat.apply(null, streams);
}


function concatModuleOffline(mPath, bPath) {
  var folders = getFolders(mPath);
  var streams = folders.map(function (folder) {
    return gulp.src([
      path.join(mPath, folder, '/localdata/*.json')])
      .pipe(gulp.dest(bPath + '/modules/localdata'));
  });
  return es.concat.apply(null, streams);
}


function copyModuleCSS(mPath, bPath) {
  var folders = getFolders(mPath);
  var streams = folders.map(function (folder) {
    return gulp.src([
      path.join(mPath, folder, '/*.css')])
    .pipe(minifyCss())
      .pipe(gulp.dest(bPath + '/modules/css'));
  });
  return es.concat.apply(null, streams);
}

function copyModuleLESS(mPath, bPath) {
  var folders = getFolders(mPath);
  console.log('updating module less');
  var streams = folders.map(function (folder) {
    return gulp.src([
      path.join(mPath, folder, '/*.less')])
      .pipe(plumber(plumberConfig))
      .pipe(less(lessConfig))
      .pipe(minifyCss())
      .pipe(gulp.dest(bPath + '/modules/css'));
  });
  return es.concat.apply(null, streams);
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
