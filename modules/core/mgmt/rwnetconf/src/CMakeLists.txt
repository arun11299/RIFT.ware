# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Tom Seidenberg
# Creation Date: 2014/05/21
# 

cmake_minimum_required(VERSION 2.8)

set(EXTRA_LIBS
    ${CMAKE_INSTALL_PREFIX}/usr/lib/libtalloc.so
    ${CMAKE_INSTALL_PREFIX}/usr/lib/librwtrace.so
)

##
# Add a shared libarary target
##
set(rwnetconf_sources
  rwnc_cbmgr.cpp
  rwnc_ds.cpp
  rwnc_filter.cpp
  rwnc_instance.cpp
  rwnc_session.cpp
  rwnc_transport.cpp
  rwnc_user.cpp
  rwnc_xml.cpp
  rwnc_xml_impl.cpp
)

add_library(rwnetconf SHARED ${rwnetconf_sources})
target_link_libraries(rwnetconf ${EXTRA_LIBS})

install(
  TARGETS rwnetconf LIBRARY
  DESTINATION usr/lib
  COMPONENT rwnetconf-1.0
)

install(
  FILES
    ../include/riftware/rwnetconf.h
  DESTINATION usr/include
  COMPONENT rwnetconf-1.0
)

install(
  FILES
    ../include/riftware/rwnc_cbmgr.hpp
    ../include/riftware/rwnc_cbmgr_pollq.hpp
    ../include/riftware/rwnc_ds.hpp
    ../include/riftware/rwnc_instance.hpp
    ../include/riftware/rwnc_session.hpp
    ../include/riftware/rwnc_transport.hpp
    ../include/riftware/rwnc_transport_direct.hpp
    ../include/riftware/rwnc_user.hpp
    ../include/riftware/rwnc_xml.hpp
  DESTINATION usr/include/c++
  COMPONENT rwnetconf-1.0
)

