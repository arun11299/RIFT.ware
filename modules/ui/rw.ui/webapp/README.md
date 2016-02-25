# RIFT.io / Punchcut collaboration area

This repo is setup for the collaboration between RIFT.io and Punchcut

* Follow the design folder for Punchcut designs and prototype code

* Follow the implementation folder for implementation of Punchcut designs


# Webpack info

##npm start
server/bundle.js starts a development environment with webpack. In memory assets can be found here: http://localhost:8080/public/
Make sure there is no public/build folder. Webpack development server will serve assets from here instead of memory.


##NODE_ENV=production webpack --config webpack.production.config.js
Creates a production bundle. Angular code needs to be refactored to be uglify friendly. After that's done you can add a "-p" flag to enable uglification.

code to be deployed will be in the public folder



#Seeding Mission Control
"cd scripts"
"node seed_mission_control.js <MC api address>"
I'll try to keep http://10.0.23.170 up, but no promises.
