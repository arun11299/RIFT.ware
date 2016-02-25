# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Creation Date: 11/26/2015
# 

cmake_minimum_required(VERSION 2.8)

if(CONFD_FLAVOUR)
  message(STATUS "Confd flavour set is ${CONFD_FLAVOUR}")

  if(CONFD_FLAVOUR STREQUAL "FULL")
    set(CONFD_DIR full)
  elseif(CONFD_FLAVOUR STREQUAL "BASIC")
    set(CONFD_DIR basic)
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
    ${CONFD_DIR}/scripts
   )

rift_add_subdirs(SUBDIR_LIST ${subdirs})
