# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Tom Seidenberg
# Creation Date: 2014/02/17
# 
set(testlibs
  rwcmdargs
  rwcmdargs_yang_gen
  rwext_yang_gen
  rwcmdargs_test_yang_gen
)

set(testdeps
  rwcmdargs_test_gen_code
)

##
# Make a shell script for running yangdump here
##
rift_yangdump_shell(
  rwcmdargs_yangdump
  NAME ${CMAKE_CURRENT_BINARY_DIR}/yangdump.sh
)

##
# Parse the yang file into an XSD and protoc-c message structure.
##
set(yang_files ${CMAKE_CURRENT_SOURCE_DIR}/rwcmdargs-test.yang)

rift_add_yang_target(
  TARGET rwcmdargs_test_yang
  YANG_FILES ${yang_files}
  NO_INSTALL
  LIBRARIES rwext_yang_gen ietf_yang_gen
  OUT_XSD_FILES_VAR yang_xsd_files
  OUT_CMDARGS_H_FILES_VAR yang_h_files
)

##
# Generate rwcmdargs C glue code functions
##
rift_cmdargs_add_target(
  rwcmdargs_test_gen_code
  DEPENDS rwcmdargs_test_yang.headers rwcmdargs_test_yang.schema
  CMDARGS_F_NAME rwcmdargs_test_gen
  PCC_TYPE RwcmdargsTest_RwcaTestCont
  XSD_FILES ${yang_xsd_files}
  YANG_FILES ${yang_files}
  PCC_H_FILES ${yang_h_files}
  OUT_C_FILES_VAR cmdargs_c_files
)

##
# Build the testing programs
##
add_executable(rwcmdargs_cli rwcmdargs_test_cli.cpp ${cmdargs_c_files})
target_link_libraries(rwcmdargs_cli PRIVATE ${testlibs})
add_dependencies(rwcmdargs_cli ${testdeps} rwcmdargs_test_yang)

##
# Testing...
#  - Unittest support
#  - Code coverage support
#  - 'make test' support, which allows a quick look at the unittest results
#  - GDB invocation support
##
rift_gtest(rwcmdargs_gtest
  TEST_SRCS rwcmdargs_test.cpp ${cmdargs_c_files}
  TEST_LIBS ${testlibs}
  DEPENDS ${testdeps}
  TEST_ARGS ${CMAKE_CURRENT_SOURCE_DIR}/rwcmdargs-test.xml
)

# GDB
add_custom_target(rwcmdargs_gdb
  COMMAND
    ${PROJECT_TOP_DIR}/scripts/env/envset.sh
      "GDB=1"
      "VERBOSE=1"
      ${CMAKE_CURRENT_BINARY_DIR}/rwcmdargs_gtest
        ${CMAKE_CURRENT_SOURCE_DIR}/rwcmdargs-test.xml
  DEPENDS rwcmdargs_gtest ${CMAKE_CURRENT_SOURCE_DIR}/rwcmdargs-test.xml
)

##
# CLI...
#  - Raw invocation
#  - GDB invocation support
##

# Raw CLI
add_custom_target(rwcmdargs_run_cli
  COMMAND
    ${PROJECT_TOP_DIR}/scripts/env/envset.sh
      ${CMAKE_CURRENT_BINARY_DIR}/rwcmdargs_cli
        ${CMAKE_CURRENT_SOURCE_DIR}/rwcmdargs-test.xml
  DEPENDS rwcmdargs_cli ${CMAKE_CURRENT_SOURCE_DIR}/rwcmdargs-test.xml
)

# GDB CLI
add_custom_target(rwcmdargs_gdb_cli
  COMMAND
    ${PROJECT_TOP_DIR}/scripts/env/envset.sh
      "GDB=1"
      "VERBOSE=1"
      ${CMAKE_CURRENT_BINARY_DIR}/rwcmdargs_cli
        ${CMAKE_CURRENT_SOURCE_DIR}/rwcmdargs-test.xml
  DEPENDS rwcmdargs_cli ${CMAKE_CURRENT_SOURCE_DIR}/rwcmdargs-test.xml
)

