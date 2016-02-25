# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Anil Gunturu
# Creation Date: 03/26/2014
# 

##
# DEPENDENCY ALERT
# The submodule dependencies must be specified in the
# .gitmodules.dep file at the top level (supermodule) directory
# If this submodule depends other submodules remember to update
# the .gitmodules.dep
##

cmake_minimum_required(VERSION 2.8)

##
# Set the path to the top level cmake modules directory
##
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake/modules")

##
# DO NOT add any code before this and DO NOT
# include this file anywhere else
##
include(rift_submodule)

##
# Submodule specific includes will go here,
# These are specified here, since these variables are accessed
# from multiple sub directories. If the variable is subdirectory
# specific it must be declared in the subdirectory.
##

if(CONFD_FLAVOUR)
  message(STATUS "Confd flavour set is ${CONFD_FLAVOUR}")

  if(CONFD_FLAVOUR STREQUAL "FULL")
    set(CONFD_DIR confd)
  elseif(CONFD_FLAVOUR STREQUAL "BASIC")
    set(CONFD_DIR confd_basic)
  else()
    message(FATAL_ERROR "Invalid value for CONFD_FLAVOUR: ${CONFD_FLAVOUR}")
  endif()

else()
  message(FATAL_ERROR "CONFD_FLAVOUR definition not found")
endif()

##
# Include the subdirs
##
set(subdirs
  libconfd
  loganalyzer
  ${CONFD_DIR}
  )
rift_add_subdirs(SUBDIR_LIST ${subdirs})

##
# This macro adds targets for documentaion, unittests, code coverage and packaging
##
rift_add_submodule_targets(SUBMODULE_PACKAGE_NAME "rw.ext.mgmt")


