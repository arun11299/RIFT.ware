# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Tom Seidenberg
# Creation Date: 2014/04/03
# 

cmake_minimum_required(VERSION 2.8)

set(subdirs yang)
rift_add_subdirs(${subdirs})

rift_install_python_plugin(testtasklet testtasklet.py)

install(
  PROGRAMS
    mgmt_tbed.py
  DESTINATION
    demos
  COMPONENT
    ${PKG_LONG_NAME}
)


rift_unittest(rwrest_module_test-1
  LONG_UNITTEST_TARGET
  TEST_ARGS
    python3 ${CMAKE_CURRENT_SOURCE_DIR}/rwrest_module_test.py
)

