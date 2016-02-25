# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Anil Gunturu
# Creation Date: 8/29/2013
# 

cmake_minimum_required(VERSION 2.8)

# RIFT-5016
rift_gtest(rwlog_gtest
  TEST_SRCS rwlog_gtest.cc
  TEST_LIBS
    rwlog
    log_test_yang_gen
    rwschema_yang_gen
    rwlib
    rt
    glib-2.0
  DEPENDS rwlog glib-2.0
)

##
# Add the rwlog python test
##
rift_pytest(rwlog_pytest
  TEST_ARGS rwlog_pytest.py
  TEST_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS rwlog rwgeneric_yang
)

