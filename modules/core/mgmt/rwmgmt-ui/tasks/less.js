var
  argv = require('yargs').argv,
  gulp = require('gulp'),
  less = require('gulp-less'),
  minifyCss = require('gulp-minify-css'),
  changed = require('gulp-changed'),
  path = require('path'),
  plumber = require('gulp-plumber')

var buildDirectory = argv.making ? argv.making : './build';
var lessConfig = {
  paths: [path.join(__dirname, 'src', 'base', 'less')]
};
var plumberConfig = {
  errorHandler: function(error) {
    console.log(error.toString())
    this.emit('end');
  }
};




gulp.task('less', function (cb) {
return gulp.src('./src/core.less')
    .pipe(plumber(plumberConfig))
    .pipe(less(lessConfig))
    .pipe(minifyCss())
    .pipe(gulp.dest('./build'))

});

gulp.task('fonts', function(){
  return gulp.src('./src/base/fonts/*.*')
  .pipe(gulp.dest('./build/modules/css/fonts'))
})

gulp.task('watch-less', ['less'], function () {
  return gulp.watch(['./src/core.less', './src/base/less/*.less', './src/integration/**/*.less'], ['less'])
});

gulp.task('watch-core-css', ['watch-less'], function(){
  return gulp.watch(['./src/core.css'], copyCSS())
});

function copyCSS() {
  console.log('Copying CSS')
  return gulp.src('./src/core.css')
  .pipe(minifyCss())
    .pipe(gulp.dest(buildDirectory + ''));
}
