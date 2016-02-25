# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Anil Gunturu
# Creation Date: 05/28/2014
# 

rift_pkg_check_modules(GOBJECT REQUIRED gobject-2.0>=2.42.0)
include_directories(BEFORE
  ${GOBJECT_INCLUDE_DIRS})


set(testlibs
  rwyang
  rwlib
  ${GOBJECT_LIBRARIES})

add_library(rwutcli SHARED rwutcli.cpp)
target_link_libraries(rwutcli PRIVATE
  ${testlibs})
add_dependencies(rwutcli rw_schema_proto)
rift_make_pc(rwutcli
  LIBS -lrwutcli
  COMPONENT rwutcli-1.0)

install(TARGETS rwutcli LIBRARY DESTINATION usr/lib 
  COMPONENT ${PKG_LONG_NAME})
install(FILES ../include/riftware/rwutcli.hpp DESTINATION usr/include/c++
  COMPONENT ${PKG_LONG_NAME})


