
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

    // Makes sure errors in console map to the correct file
    // and line number
    devtool: 'eval',
    entry:  [

            // For hot style updates
            'webpack/hot/dev-server',

            // The script refreshing the browser on none hot updates
            'webpack-dev-server/client?http://localhost:8080',

            // Our application
            mainPath
        ]

    ,
    output: {
        // We need to give Webpack a path. It does not actually need it,
        // because files are kept in memory in webpack-dev-server, but an
        // error will occur if nothing is specified. We use the buildPath
        // as that points to where the files will eventually be bundled
        // in production
        path: buildPath,
        filename: 'bundle.js',
        // Everything related to Webpack should go through a build path,
        // localhost:3000/build. That makes proxying easier to handle
        publicPath: 'http://localhost:8000/build/'
    },
    module: {
        // preLoaders: [{
        //     test: /\.js$/,
        //     // loader: 'baggage?[file].html&[file].css'
        //     loader: 'baggage?[file].html'
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
                loader: 'style!css?sourceMap'
            }, {
                test: /\.scss/,
                loader: 'style!css?sourceMap!sass'
            }
        ]
    },

    // We have to manually add the Hot Replacement plugin when running
    // from Node
    plugins: [
    new Webpack.HotModuleReplacementPlugin()
    , new Webpack.optimize.CommonsChunkPlugin("vendor", "vendor.js", Infinity)
    ]
};

module.exports = config;
