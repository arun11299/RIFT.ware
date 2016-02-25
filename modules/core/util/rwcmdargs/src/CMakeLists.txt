# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Tom Seidenberg
# Creation Date: 2014/02/05
# 

rift_add_doxygen_target(rwcmdargs_doxygen DEST_DIR rwcmdargs)

rift_pkg_check_modules(GLIB REQUIRED glib-2.0>=2.42.0)

add_library(rwcmdargs SHARED rwcmdargs.cpp)
add_dependencies(rwcmdargs rwcmdargs_yang_gen)
target_link_libraries(rwcmdargs PRIVATE
  rwlib
  rwyang
  ${GLIB_LIBRARIES})

include_directories(BEFORE
  ${GLIB_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${RIFT_SUBMODULE_BINARY_ROOT}/yangtools/yang
  ${RIFT_SUBMODULE_BINARY_ROOT}/rwcmdargs/yang)

install(FILES ../include/riftware/rwcmdargs.hpp DESTINATION usr/include/c++ COMPONENT ${PKG_LONG_NAME})
install(FILES ../include/riftware/rwcmdargs.h DESTINATION usr/include COMPONENT ${PKG_LONG_NAME})
install(TARGETS rwcmdargs LIBRARY DESTINATION usr/lib COMPONENT ${PKG_LONG_NAME})

rift_make_pc(rwcmdargs
  LIBS -lrwcmdargs
  INCLUDEDIRS ${GLIB_INCLUDE_DIRS}
  COMPONENT rwcmdargs-1.0)

