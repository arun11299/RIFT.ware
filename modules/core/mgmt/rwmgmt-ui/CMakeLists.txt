# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Laurence Maultsby
# Creation Date: 03/19/2015
# 

MESSAGE(STATUS "Building UI....")

add_custom_target(rw.mgmt.ui.gui ALL
  DEPENDS ${CMAKE_CURRENT_BUILD_DIR}/rwmgmt-ui/main.js
  )

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BUILD_DIR}/rwmgmt-ui/main.js
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/node_modules/.bin/gulp build --making ${CMAKE_CURRENT_BINARY_DIR}/rwmgmt-ui
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

install(
  DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/rwmgmt-ui
  DESTINATION usr/share
  COMPONENT RWMGMT
  )
