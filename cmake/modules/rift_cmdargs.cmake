# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Tom Seidenberg
# Creation Date: 2014/02/13
# 
# This file contains cmake macros for generating documentation

include(CMakeParseArguments)
include(rift_yang)

##
# This function adds target for processing a cmdargs yang file into all the necessary derived files.
# rift_cmdargs_add_target(
#   <target>
#   DEPENDS <target-dependencies> ...
#   CMDARGS_F_NAME <generated-code-function-name>
#   PCC_TYPE <output-protobuf-c-message-type>
#   DEST_DIR <destination-dir>
#   XSD_FILES <manifest-xsd-files> ...
#   YANG_FILES <manifest-yang-files> ...
#   PCC_H_FILES <manifest-protocc-h-files> ...
#   INSTALL_COMP <install-component-for-yang-xsd-proto-and-h-files>
#   INSTALL_H_DIR <h-file-install-dir>
#   OUT_C_FILES_VAR <name-of-the-var-to-hold-c-file-list>
# )
##
function(rift_cmdargs_add_target target)
  set(parse_options)
  set(parse_multivalueargs DEPENDS XSD_FILES YANG_FILES PCC_H_FILES)
  set(parse_onevalargs CMDARGS_F_NAME PCC_TYPE DEST_DIR INSTALL_COMP INSTALL_H_DIR OUT_C_FILES_VAR)
  cmake_parse_arguments(ARG "${parse_options}" "${parse_onevalargs}" "${parse_multivalueargs}" ${ARGN})

  if(NOT ARG_DEST_DIR)
    set(ARG_DEST_DIR ${CMAKE_CURRENT_BINARY_DIR})
  endif()

  if(NOT INSTALL_H_DIR)
    set(INSTALL_H_DIR usr/include)
  endif()

  if(ARG_INSTALL_COMP)
    set(cmdargs_inst ${ARG_INSTALL_COMP})
  else()
    set(cmdargs_inst ${target})
  endif()

  # Generate glue code files,
  set(args_base ${ARG_DEST_DIR}/${ARG_CMDARGS_F_NAME})
  set(args_c_file ${args_base}.c)
  set(args_h_file ${args_base}.h)
  add_custom_command(
    OUTPUT ${args_c_file} ${args_h_file}
    COMMAND ${PROJECT_TOP_DIR}/scripts/util/cmdargs.pl
      BASE ${args_base}
      CMDARGS_F_NAME ${ARG_CMDARGS_F_NAME}
      INSTALL_COMP ${cmdargs_inst}
      PCC_TYPE ${ARG_PCC_TYPE}
      PCC_H_FILES ${ARG_PCC_H_FILES}
      XSD_FILES ${ARG_XSD_FILES}
      YANG_FILES ${ARG_YANG_FILES}
    DEPENDS ${PROJECT_TOP_DIR}/scripts/util/cmdargs.pl ${ARG_DEPENDS} ${ARG_YANG_FILES}
  )
  add_custom_target(${target} DEPENDS ${args_c_file} ${args_h_file})

  if(ARG_INSTALL_COMP)
    install(
      FILES ${args_h_file}
      DESTINATION ${ARG_INSTALL_H_DIR}
      COMPONENT ${ARG_INSTALL_COMP})
  endif()

  set(${ARG_OUT_C_FILES_VAR} ${args_c_file} PARENT_SCOPE)

endfunction(rift_cmdargs_add_target)

