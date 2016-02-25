# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Tome Seidenberg
# Creation Date: 2014/02/13
# 


rift_pkg_check_modules(PROTOBUF_C REQUIRED libprotobuf-c>=1.1.0)

include_directories(BEFORE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${RIFT_SUBMODULE_BINARY_ROOT}/yangtools/yang
)

set(yang_sources rwcmdargs-min.yang rwcmdargs-ext.yang rwcmdargs-cli.yang)

rift_add_yang_target(
  TARGET rwcmdargs_yang
  YANG_FILES ${yang_sources}
  COMPONENT ${PKG_LONG_NAME}
  LIBRARIES rwext_yang_gen ietf_yang_gen
)

