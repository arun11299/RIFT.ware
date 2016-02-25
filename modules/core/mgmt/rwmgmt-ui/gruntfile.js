module.exports = function(grunt) {

  grunt.initConfig({
    pkg: grunt.file.readJSON('package.json'),
    svgstore: {
      options: {
        prefix : 'icon-', // This will prefix each ID
        svg: { // will be added as attributes to the resulting SVG
          viewBox : '0 0 100 100'
        }
      },
      default : {
        files: {
          'icons/icons.svg': ['icons/svg/*.svg'],
        },
      },
    },
  });

  grunt.loadNpmTasks('grunt-svgstore');

  // Default task(s).
  grunt.registerTask('default', ['svgstore']);
};