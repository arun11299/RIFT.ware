# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Anil Gunturu
# Creation Date: 03/26/2014
# 

cmake_minimum_required(VERSION 2.8)

set(PKG_NAME rwmano_examples)
set(PKG_VERSION 1.0)
set(PKG_RELEASE 1)
set(PKG_LONG_NAME ${PKG_NAME}-${PKG_VERSION})


##
# Include the subdirs
##
set(subdirs
  ping_pong_ns
  )

rift_add_subdirs(SUBDIR_LIST ${subdirs})
