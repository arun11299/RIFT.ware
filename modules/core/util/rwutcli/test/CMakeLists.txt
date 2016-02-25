# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Anil Gunturu
# Creation Date: 05/28/2014
# 

add_library(test_rwutcli SHARED test_rwutcli.cpp)
add_dependencies(test_rwutcli rw_schema_pb)

set(rwutcli_test_yang_files test_rwutcli.yang)
rift_add_yang_target(
  TARGET rwutcli_test_yang
  YANG_FILES ${rwutcli_test_yang_files}
  WITHOUT_GI
  NO_INSTALL
  LIBRARIES
    test_rwutcli
    protobuf-c
    rwutcli
    rwyang
    rwext_yang_gen
    ietf_yang_gen
  DEPENDS rwutcli_base_yang.headers rw_schema_pb
)

add_executable(test_main test_main.cpp)
add_definitions(-DRW_YANGPBC_ENABLE_UTCLI)
add_dependencies(test_main rwutcli_test_yang.headers)
target_link_libraries(test_main PRIVATE
  rwutcli_test_yang_gen)

# Add a gdb target for calling test_main
add_custom_target(test_cli
  COMMAND
  ${PROJECT_TOP_DIR}/scripts/env/envset.sh
    "GDB=1"
    "VERBOSE=1"
    ${CMAKE_CURRENT_BINARY_DIR}/test_main
  DEPENDS test_main)

