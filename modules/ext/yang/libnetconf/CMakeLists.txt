include(rift_externalproject)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(PKG_DEBUG_FLAGS "CFLAGS=-g -O0 -I${CMAKE_INSTALL_PREFIX}/usr/include -I<BINARY_DIR>/src -I<BINARY_DIR>/headers")
endif()

rift_externalproject_add(
  libnetconf
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libnetconf-0.7.99
  CONFIGURE_COMMAND
    ./configure
      --prefix=${CMAKE_INSTALL_PREFIX}/usr
      --with-ncworkingdirpath=${CMAKE_INSTALL_PREFIX}/var/lib/libnetconf
      "CFLAGS=-I${CMAKE_INSTALL_PREFIX}/usr/include -I<BINARY_DIR>/src -I<BINARY_DIR>/headers"
      LDFLAGS=-L${CMAKE_INSTALL_PREFIX}/usr/lib
      PKG_CONFIG_PATH=${CMAKE_INSTALL_PREFIX}/usr/lib/pkgconfig
      ${PKG_DEBUG_FLAGS}
  DEPENDS pyang

)

