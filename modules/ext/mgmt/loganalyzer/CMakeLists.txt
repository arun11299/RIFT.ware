# RIFT_IO_STANDARD_CMAKE_COPYRIGHT_HEADER(BEGIN)
# Author(s): Anil Gunturu
# Creation Date: 03/26/2014
# RIFT_IO_STANDARD_CMAKE_COPYRIGHT_HEADER(END)

##
# DEPENDENCY ALERT
# The submodule dependencies must be specified in the
# .gitmodules.dep file at the top level (supermodule) directory
# If this submodule depends other submodules remember to update
# the .gitmodules.dep
##

cmake_minimum_required(VERSION 2.8)

##
# Submodule specific includes will go here, 
# These are specified here, since these variables are accessed
# from multiple sub directories. If the variable is subdirectory
# specific it must be declared in the subdirectory.
##

set(PKG_NAME loganalyzer)
set(PKG_VERSION 4.1.3)
set(PKG_RELEASE 1)
set(PKG_LONG_NAME ${PKG_NAME}-${PKG_VERSION})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/loganalyzer-4.1.3/src/
  DESTINATION usr/lib/rift/html/loganalyzer
  COMPONENT ${PKG_LONG_NAME}
  )

set(SCRIPT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../../scripts/cloud")

install(PROGRAMS ${SCRIPT_PATH}/install_loganalyzer
  DESTINATION etc
  COMPONENT ${PKG_LONG_NAME}
  )
