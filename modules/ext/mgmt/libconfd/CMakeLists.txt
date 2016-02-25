include(rift_externalproject)

set(libconfd_version 6.0)


if(CONFD_FLAVOUR)
  message(STATUS "Confd flavour set is ${CONFD_FLAVOUR}")

  if(CONFD_FLAVOUR STREQUAL "FULL")
    set(LIBCONFD_PACKAGE confd-${libconfd_version}.libconfd.tar.gz)
    set(LIBCONFD_DIR libconfd-${libconfd_version})
    set(INTERNAL_CONFD_DIR confd-${libconfd_version})
  elseif(CONFD_FLAVOUR STREQUAL "BASIC")
    set(LIBCONFD_PACKAGE confd-basic-${libconfd_version}.libconfd.tar.gz)
    set(LIBCONFD_DIR libconfd-basic-${libconfd_version})
    set(INTERNAL_CONFD_DIR confd-basic-${libconfd_version})
  else()
    message(FATAL_ERROR "Invalid value for CONFD_FLAVOUR: ${CONFD_FLAVOUR}")
  endif()

else()
  message(FATAL_ERROR "CONFD_FLAVOUR definition not found")
endif()

rift_externalproject_add(
  libconfd
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${LIBCONFD_DIR}
  CONFIGURE_COMMAND tar -xvf ${LIBCONFD_PACKAGE}
  BUILD_COMMAND
  make -C ${INTERNAL_CONFD_DIR}/libconfd CONFD_DIR=${CMAKE_INSTALL_PREFIX}/usr/local/confd
  COMMAND
  make -C ${INTERNAL_CONFD_DIR}/libconfd crypto PREFIX=${CMAKE_INSTALL_PREFIX}
  BCACHE_COMMAND echo
  INSTALL_COMMAND echo
)

set(install_path ${CMAKE_INSTALL_PREFIX}/usr)
externalproject_add_step(
  libconfd
  real_install
  COMMAND mkdir -p ${install_path}/local/confd/lib
  COMMAND cp <BINARY_DIR>/${INTERNAL_CONFD_DIR}/libconfd/src/libconfd.so
          ${install_path}/local/confd/lib/
  COMMAND mkdir -p ${install_path}/local/confd/lib/confd/lib/core/crypto/priv/lib/
  COMMAND cp <BINARY_DIR>/${INTERNAL_CONFD_DIR}/libconfd/crypto/crypto.so
          ${install_path}/local/confd/lib/confd/lib/core/crypto/priv/lib/
  DEPENDEES build
  DEPENDERS install
)

set(bcache_path
  ${RIFT_SUBMODULE_INSTALL_PREFIX}/libconfd/${CMAKE_INSTALL_PREFIX}/usr)
externalproject_add_step(
  libconfd
  real_bcache
  COMMAND mkdir -p ${bcache_path}/local/confd/lib
  COMMAND cp <BINARY_DIR>/${INTERNAL_CONFD_DIR}/libconfd/src/libconfd.so
          ${bcache_path}/local/confd/lib/
  COMMAND mkdir -p ${bcache_path}/local/confd/lib/confd/lib/core/crypto/priv/lib/
  COMMAND cp <BINARY_DIR>/${INTERNAL_CONFD_DIR}/libconfd/crypto/crypto.so
          ${bcache_path}/local/confd/lib/confd/lib/core/crypto/priv/lib/
  DEPENDEES build
  DEPENDERS bcache
)

