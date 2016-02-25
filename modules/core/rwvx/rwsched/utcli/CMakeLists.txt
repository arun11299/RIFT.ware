# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Tom Seidenberg
# Creation Date: 2014/06/05
# 

set(libs
  rwsched
  rwlog
  rwutcli)

add_library(rwsched_utcli SHARED rwsched_utcli.cpp)
add_definitions(-DRW_YANGPBC_ENABLE_UTCLI)
target_link_libraries(rwsched_utcli PRIVATE
  rwlib
  rwlog
  rwsched
  rwutcli
  rwvcs
  rwyang
  CoreFoundation
  glib-2.0)

install(TARGETS rwsched_utcli LIBRARY DESTINATION usr/lib
  COMPONENT rwsched_utcli-1.0)
install(FILES ../include/riftware/rwsched_utcli.hpp DESTINATION usr/include/c++
  COMPONENT rwsched_utcli-1.0)

