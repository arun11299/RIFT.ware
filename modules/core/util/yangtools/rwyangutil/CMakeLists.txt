# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Arun Muralidharan
# Creation Date: 2015/10/01
# 

include(rift_pkg_config)
# rw_file_proto_ops is dependednt on some 
# rift libraries. It is meant to be run in conjunction
# with rift-ware.

cmake_minimum_required(VERSION 2.8)

add_definitions(-std=c++11)

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})

find_library(boost_filesystem_path NAMES boost_filesystem)
if (NOT boost_filesystem_path)
  message (FATAL_ERROR "Could not find library ${boost_filesystem_path}")
endif ()
find_library(boost_system_path NAMES boost_system)
if (NOT boost_system_path)
  message (FATAL_ERROR "Could not find library ${boost_system_path}")
endif ()

add_executable(rwyangutil rwyangutil.cc)

target_link_libraries(rwyangutil
  ${boost_filesystem_path}
  ${boost_system_path}
)

install(TARGETS rwyangutil
 DESTINATION usr/bin
 COMPONENT ${PKG_LONG_NAME})

install(FILES rwyangutil.h DESTINATION usr/include COMPONENT ${PKG_LONG_NAME})

add_subdirectory(test)
