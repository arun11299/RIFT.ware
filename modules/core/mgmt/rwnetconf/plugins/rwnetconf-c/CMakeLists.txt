# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Tom Seidenberg
# Creation Date: 2014/05/21
# 

##
# Set some RIFT specific variables for this cmake project
##
set(PLUGIN_NAME rwnetconf_client-c)

##
# Set the name of the Vala source for this cmake project; for use later
# in this file
##
set(C_SOURCES rwnetconf_client-c.c)

##
# Add the given directories to those searched by the compiler for
# include files. By default the directories are appended onto the
# current list of directories
##
include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR}/../../vala)

##
# Adds a library target to be built from the source files listed in the
# command invocation
##
add_library(${PLUGIN_NAME} SHARED ${C_SOURCES})

##
# Specify libraries or flags to use when linking a given target.  The
# target must have been created in the current directory by a command
# such as add_executable or add_library
##
target_link_libraries(${PLUGIN_NAME} ${RIFT_SUBMODULE_BINARY_ROOT}/rwnetconf/src/librwnetconf.so)

##
# libpeas must be compiled before this target
##
add_dependencies(${PLUGIN_NAME} rwnetconf)

##
# This function creates a vapi2c target to generate Vala source/header
# files from .in files
##
rift_add_plugin_vapi2c(
  ${PLUGIN_NAME}.vapi2c
  VAPI_FILE ../../vala/rwnetconf_client-1.0.vapi
  PLUGIN_SOURCE_PREFIX rwnetconf_client-c
  DEPENDS rwnetconf_client-1.0
  )

##
# This function builds the plugin schema files
##
rift_add_c_plugin_schema(
  ${PLUGIN_NAME}
  GSCHEMA ${PLUGIN_NAME}.gschema.xml
  PLUGIN ${PLUGIN_NAME}.plugin
  )

##
# This function creates an install target for the plugin artifacts
##
rift_install_plugin_artifacts(
  ${PLUGIN_NAME}
  SOTARGETS ${PLUGIN_NAME}
  GSCHEMAFILES ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}.gschema.xml
  PLUGINFILES ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}.plugin
  COMPONENT rwnetconf-1.0
  )
