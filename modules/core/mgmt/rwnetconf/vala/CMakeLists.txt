# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Tom Seidenberg
# Creation Date: 2014/05/21
# 

##
# Include the rift Vala and plugin functions/macros
##
include(rift_plugin)
include(FindVala)
include(UseVala)

##
# Allow specific compiler warnings
##
rift_allow_compiler_warning(unused-but-set-variable)
rift_allow_compiler_warning(unused-parameter)

##
# Add the given directories to those searched by the compiler for
# include files.  By default the directories are appended onto the
# current list of directories.
##
include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

##
# Specify the paths in which the linker should search for libraries.
# The command will apply only to targets created after it is called.
##
link_directories(${CMAKE_CURRENT_BINARY_DIR})

##
# rwnetconf_client-1.0
#
# Set some RIFT specific VALA variables for this cmake project
##
set(VALA_FILES rwnetconf_client.vala)
set(VALA_NAME rwnetconf_client)
set(VALA_VERSION 1.0)
set(VALA_RELEASE 1)
set(VALA_LONG_NAME ${VALA_NAME}-${VALA_VERSION})
set(VALA_TYPELIB_PREFIX YangModelPlugin-${VALA_VERSION})

##
# rwnetconf_client-1.0
#
# This function compiles all the vala sources
# Generates the package .h/.c sources, .gir file, .vapi file, and the .so
##
rift_add_vala(
  ${VALA_LONG_NAME}
  VALA_FILES ${VALA_FILES}
  GENERATE_HEADER_FILE ${VALA_NAME}.h
  GENERATE_SO_FILE lib${VALA_LONG_NAME}.so
  GENERATE_VAPI_FILE ${VALA_LONG_NAME}.vapi
  GENERATE_GIR_FILE ${VALA_TYPELIB_PREFIX}.gir
  GENERATE_TYPELIB_FILE ${VALA_TYPELIB_PREFIX}.typelib
  DEPENDS libpeas-${LIBPEAS_VERSION}
  )

##
# rwnetconf_client-1.0
#
# This function creates an install target for artifacts generated from the vala files
##
rift_install_vala_artifacts(
  HEADER_FILES ${VALA_NAME}.h
  SO_FILES lib${VALA_LONG_NAME}.so
  VAPI_FILES ${VALA_LONG_NAME}.vapi
  GIR_FILES ${VALA_TYPELIB_PREFIX}.gir
  TYPELIB_FILES ${VALA_TYPELIB_PREFIX}.typelib
  COMPONENT rwnetconf-1.0
  DEST_PREFIX .
  )
