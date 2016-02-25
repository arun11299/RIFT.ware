# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Tom Seidenberg
# Creation Date: 2014/05/21
# 

cmake_minimum_required(VERSION 2.8)

##
# Add flags to the compiler command line for sources in the current
# directory
##
add_definitions(-DINSTALLDIR="${CMAKE_INSTALL_PREFIX}")

##
# Add the given directories to those searched by the compiler for
# include files.  By default the directories are appended onto the
# current list of directories.
##
include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

##
# Generate the protobuf file
##
set(proto_files rwnetconf_xapi_rwmsg.proto)
rift_add_proto_target(
  TARGET rwnetconf_proto_parse
  WITH_DSO
  PROTO_FILES ${proto_files}
  OUT_C_FILES_VAR proto_c_files
  COMPONENT ${PKG_LONG_NAME}
)

##
# Add the target for the rwnetconf library
##
add_library(rwnetconfproto SHARED ${proto_c_files})
install(TARGETS rwnetconfproto LIBRARY DESTINATION usr/lib COMPONENT ${PKG_LONG_NAME})

