
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
var Webpack = require('webpack');
var path = require('path');
var nodeModulesPath = path.resolve(__dirname, 'node_modules');
var buildPath = path.resolve(__dirname, 'public', 'build');
var mainPath = path.resolve(__dirname, 'app', 'main.js');

var config = {

  // We change to normal source mapping
  devtool: 'source-map',
  entry: mainPath,
  output: {
    path: buildPath,
    filename: 'bundle.js',
    publicPath: "/build/"
  },
  module: {
    // preLoaders: [{
    //   test: /\.js$/,
    //   loader: 'baggage?[file].html&[file].css'
    // }],
    loaders: [{
                test: /\.html$/,
                loader: "ngtemplate?relativeTo=" + (path.resolve(__dirname, 'app/')) + "/&prefix=/!html"
            }, {
                test: /\.(jpe?g|png|gif|svg|ttf|otf|eot|svg|woff(2)?)(\?[a-z0-9]+)?$/i,
                loader: "file-loader"
            },
            // I highly recommend using the babel-loader as it gives you
            // ES6/7 syntax and JSX transpiling out of the box
            {
                test: /\.(js|jsx)$/,
                exclude: [nodeModulesPath],
                loader: 'babel-loader',
                query: {stage: 0}
            },
            {
                test: /\.css$/,
                loader: 'style!css'
            }, {
                test: /\.scss/,
                loader: 'style!css!sass'
            }
        ]
  },
  plugins: [
    new Webpack.optimize.CommonsChunkPlugin("vendor", "vendor.js", Infinity)
    ]
};

module.exports = config;
