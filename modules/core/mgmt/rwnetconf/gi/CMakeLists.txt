# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Tom Seidenberg
# Creation Date: 2014/05/21
# 

##
# Add the given directories to those searched by the compiler for
# include files.  By default the directories are appended onto the
# current list of directories.
##
include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

##
# Specify the paths in which the linker should search for libraries.
# The command will apply only to targets created after it is called.
##
link_directories(${CMAKE_CURRENT_BINARY_DIR})

set(testlibs
  ${CMAKE_INSTALL_PREFIX}/usr/lib/libncx.so
  ${CMAKE_INSTALL_PREFIX}/usr/lib64/libxml2.so
  ${CMAKE_INSTALL_PREFIX}/usr/lib/librwlib.so
  ${CMAKE_INSTALL_PREFIX}/usr/lib/librwyang.so
  ${CMAKE_INSTALL_PREFIX}/usr/lib/libxerces-c.so
  ${CMAKE_INSTALL_PREFIX}/usr/lib/libprotobuf-c.so
  ${CMAKE_INSTALL_PREFIX}/usr/lib/libedit.so
  ${CMAKE_INSTALL_PREFIX}/usr/lib/libglib-2.0.so
  ${CMAKE_INSTALL_PREFIX}/usr/lib/libCoreFoundation.so
  ${CMAKE_INSTALL_PREFIX}/usr/lib/librwtrace.so
#  ${RIFT_SUBMODULE_BINARY_ROOT}/rwnetconf/test/proto/librwnetconf_test_proto.a
#  ${RIFT_SUBMODULE_BINARY_ROOT}/rwnetconf/test/yang/librwnetconf_test_yang.a
  ${RIFT_SUBMODULE_BINARY_ROOT}/rwnetconf/src/librwnetconf.so
  ${CMAKE_INSTALL_PREFIX}/usr/lib/libtalloc.so
  )


set(GI_NAME rwnetconf_gi)
set(GI_VERSION 1.0)
set(GI_NAMESPACE RwNetconf)

add_library(${GI_NAME} SHARED rwnetconf_gi.c)
target_link_libraries(${GI_NAME} ${RIFT_GLOBAL_LIBPEAS_LIBRARIES} ${testlibs})
add_dependencies(${GI_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/rwnetconf_gi_filter.py)

install(TARGETS ${GI_NAME} LIBRARY DESTINATION usr/lib
  COMPONENT ${PKG_LONG_NAME})

rift_add_introspection(${GI_NAME}_gi
  NAMESPACE ${GI_NAMESPACE}
  VERSION ${GI_VERSION}
  SYMBOL_PREFIX rw_ncclnt
  IDENTIFIER_FILTER_CMD "python ${CMAKE_CURRENT_SOURCE_DIR}/rwnetconf_gi_filter.py"

  CFILES ${CMAKE_CURRENT_SOURCE_DIR}/rwnetconf_gi.c
  HFILES ${CMAKE_CURRENT_SOURCE_DIR}/rwnetconf_gi.h

  INCLUDE_PATHS ${CMAKE_INSTALL_PREFIX}/usr/include

  LIBRARIES ${GI_NAME} 
    ncx xml2 rwlib rwyangxerces-c protobuf-c edit glib-2.0
    CoreFoundation rwtrace rwnetconf talloc
  DEPENDS ${GI_NAME}
  GENERATE_GI_OVERRIDE
  )
add_dependencies(${GI_NAME}_ rwnetconf_gi)


add_custom_target(rwnetconf_gi_test
  COMMAND
  ${PROJECT_TOP_DIR}/scripts/env/envset.sh
  python ${CMAKE_CURRENT_SOURCE_DIR}/rwnetconf_gi_test.py
  )

