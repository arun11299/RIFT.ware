# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Tom Seidenberg
# Creation Date: 2014/05/21
# 

set(plugindir "${CMAKE_CURRENT_BINARY_DIR}/../plugins")
add_definitions(-DPLUGINDIR="${plugindir}")
add_definitions(-DINSTALLDIR="${CMAKE_INSTALL_PREFIX}")

##
# Converted unit test target rwnetconf_gtest (runs with GTest and Celero)
##
set(testdeps
  rwnetconf
)

set(testlibs
  ${CMAKE_INSTALL_PREFIX}/usr/lib/libncx.so
  ${CMAKE_INSTALL_PREFIX}/usr/lib/librwlib.so
  ${CMAKE_INSTALL_PREFIX}/usr/lib/librwyang.so
  ${CMAKE_INSTALL_PREFIX}/usr/lib/libxerces-c.so
  ${CMAKE_INSTALL_PREFIX}/usr/lib/libprotobuf-c.so
  ${CMAKE_INSTALL_PREFIX}/usr/lib/libedit.so
  ${CMAKE_INSTALL_PREFIX}/usr/lib/libglib-2.0.so
  ${CMAKE_INSTALL_PREFIX}/usr/lib/libCoreFoundation.so
  ${CMAKE_INSTALL_PREFIX}/usr/lib/librwtrace.so
  ${RIFT_SUBMODULE_BINARY_ROOT}/rwnetconf/src/librwnetconf.so
  ${CMAKE_INSTALL_PREFIX}/usr/lib/libtalloc.so

  # System libraries
  xml2
)

set(GTEST_C_SOURCES
  test_rwnetconf_client.cc
  server_mock.cpp
  transport_null.cpp
)

rift_gtest(rwnetconf_gtest
  ADD_SETUP
  TEST_SRCS ${GTEST_C_SOURCES}
  TEST_LIBS ${testlibs}
  DEPENDS ${testdeps}
)

# Add a gdb target for "handy" debugging
add_custom_target(rwnetconf_gdb
  COMMAND
    ${PROJECT_TOP_DIR}/scripts/env/envset.sh
      "GDB=1"
      "VERBOSE=1"
      ${CMAKE_CURRENT_BINARY_DIR}/rwnetconf_gtest
  DEPENDS rwnetconf_gtest
)

