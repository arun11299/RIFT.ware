# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Anil Gunturu
# Creation Date: 03/26/2014
# 

cmake_minimum_required(VERSION 2.8)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/generate_packages.sh.in
  ${CMAKE_CURRENT_BINARY_DIR}/generate_packages.sh
  ESCAPE_QUOTES @ONLY
  )


#add_custom_target(ping_pong_qcow2_images ALL
#  DEPENDS
#    ${CMAKE_CURRENT_BINARY_DIR}/ping_pong/Fedora-x86_64-20-20131211.1-sda-ping.qcow2
#    ${CMAKE_CURRENT_BINARY_DIR}/ping_pong/Fedora-x86_64-20-20131211.1-sda-pong.qcow2
#  )

set(PACKAGE_OUTPUT
  ${CMAKE_CURRENT_BINARY_DIR}/ping_pong_nsd.tar.gz
  ${CMAKE_CURRENT_BINARY_DIR}/ping_vnfd.tar.gz
  ${CMAKE_CURRENT_BINARY_DIR}/pong_vnfd.tar.gz
  ${CMAKE_CURRENT_BINARY_DIR}/ping_pong_nsd_with_epa.tar.gz
  ${CMAKE_CURRENT_BINARY_DIR}/ping_vnfd_with_epa.tar.gz
  ${CMAKE_CURRENT_BINARY_DIR}/pong_vnfd_with_epa.tar.gz)

set(BASE_IMAGE ${PROJECT_TOP_DIR}/images/Fedora-x86_64-20-20131211.1-sda.qcow2)
execute_process(
  COMMAND "test" "-e" ${BASE_IMAGE}
  OUTPUT_VARIABLE out
  RESULT_VARIABLE result
  ERROR_VARIABLE err
)

if (${result} EQUAL 0)

#if(EXISTS(${PROJECT_TOP_DIR}/images/Fedora-x86_64-20-20131211.1-sda.qcow2))
  set(PACKAGE_OUTPUT ${PACKAGE_OUTPUT}
    ${CMAKE_CURRENT_BINARY_DIR}/ping_vnfd_with_image.tar.gz
    ${CMAKE_CURRENT_BINARY_DIR}/pong_vnfd_with_image.tar.gz)

  add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/ping_pong/Fedora-x86_64-20-20131211.1-sda-ping.qcow2
      ${CMAKE_CURRENT_BINARY_DIR}/ping_pong/Fedora-x86_64-20-20131211.1-sda-pong.qcow2
    COMMAND
      ${CMAKE_CURRENT_SOURCE_DIR}/ping_pong_ns/prepare_ping_pong_qcow.sh
      ${CMAKE_CURRENT_SOURCE_DIR}/ping_pong_ns
    DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/ping_pong_ns/prepare_ping_pong_qcow.sh
      ${CMAKE_CURRENT_SOURCE_DIR}/ping_pong_ns/ping.py
      ${CMAKE_CURRENT_SOURCE_DIR}/ping_pong_ns/pong.py
      ${CMAKE_CURRENT_SOURCE_DIR}/ping_pong_ns/start_ping.py
      ${CMAKE_CURRENT_SOURCE_DIR}/ping_pong_ns/start_pong.py
    )

  add_custom_command(
    OUTPUT ${PACKAGE_OUTPUT}
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/generate_packages.sh
    DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/ping_pong_ns/prepare_ping_pong_qcow.sh
      ${CMAKE_CURRENT_SOURCE_DIR}/ping_pong_ns/ping.py
      ${CMAKE_CURRENT_SOURCE_DIR}/ping_pong_ns/pong.py
      ${CMAKE_CURRENT_SOURCE_DIR}/ping_pong_ns/start_ping.py
      ${CMAKE_CURRENT_SOURCE_DIR}/ping_pong_ns/start_pong.py
      ${CMAKE_CURRENT_SOURCE_DIR}/ping_pong_nsd.py
  )

  add_custom_target(
      ping_pong_images
      DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/ping_pong/Fedora-x86_64-20-20131211.1-sda-ping.qcow2
        ${CMAKE_CURRENT_BINARY_DIR}/ping_pong/Fedora-x86_64-20-20131211.1-sda-pong.qcow2
      )

  add_custom_target(
      ping_pong_packages
      DEPENDS
        ${PACKAGE_OUTPUT}
      )

  add_dependencies(ping_pong_packages ping_pong_images mano_yang)

  add_custom_target(ping_pong_pkg_gen ALL
    DEPENDS ping_pong_packages
  )

  install(
    FILES ${PACKAGE_OUTPUT}
    DESTINATION
      usr/rift/mano/examples/ping_pong_ns
      COMPONENT ${PKG_LONG_NAME}
    )

else()
  message("Missing base image: ${BASE_IMAGE}")
  add_custom_command(
    OUTPUT ${PACKAGE_OUTPUT}
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/generate_packages.sh
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ping_pong_nsd.py
  )

  add_custom_target(ping_pong_pkg_gen ALL
    DEPENDS ${PACKAGE_OUTPUT}
    mano_yang
  )

  install(
    FILES ${PACKAGE_OUTPUT}
    DESTINATION
      usr/rift/mano/examples/ping_pong_ns
      COMPONENT ${PKG_LONG_NAME}
    )
endif()

rift_python_install_tree(
  COMPONENT ${PKG_LONG_NAME}
  FILES
    rift/mano/examples/ping_pong_nsd.py
  )

