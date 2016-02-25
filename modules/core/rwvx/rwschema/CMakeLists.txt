# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Tim Mortsolf
# Creation Date: 2014/03/13
# 

cmake_minimum_required(VERSION 2.8)

set(PKG_NAME rwschema)
set(PKG_VERSION 1.0)
set(PKG_RELEASE 1)
set(PKG_LONG_NAME ${PKG_NAME}-${PKG_VERSION})

rift_allow_compiler_warning(unused-parameter)
rift_allow_compiler_warning(missing-field-initializers)

set(subdirs yang)
rift_add_subdirs(SUBDIR_LIST ${subdirs})
