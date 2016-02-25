# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Austin Cormier
# Creation Date: 4/3/2015
# 

cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/modules")

# rift_submodule.yang assumes this CMakeLists.txt is within the modules/*.
# Since rift_submodule.cmake is a dependency for including rift_yang.cmake,
# I decided to put this CMakeLists.txt within modules/yang_composite.
# This is then the first "virtual" submodule which is actually not a real
# git submodule but allows all build code paths to remain the same.
include(rift_submodule)

include(rift_globals)
include(rift_build_utils)
include(rift_externalproject)
include(rift_yang)

# Because we are not in a yang/ subdirectory, the CURRENT_BINARY_DIR is not
# automatically added to the include list.
include_directories(BEFORE "${CMAKE_CURRENT_BINARY_DIR}")

# Only generate the
if (EXISTS ${CMAKE_INSTALL_PREFIX}/usr/bin/yangpbc AND
    EXISTS "${CMAKE_CURRENT_BINARY_DIR}/../../../../rw-composite.yang")

  # Copy the generated rw-composite.yang from the .build/ directory
  # to this current binary directory since yangpbc really wants the
  # yang file to be in the YUMA_MODPATH.
  add_custom_target(copy_rw_composite_yang
    COMMAND cp -f
    ${CMAKE_CURRENT_BINARY_DIR}/../../../../rw-composite.yang
    ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Copying rw_composite.yang."
  )

  # By the time this is reached, the rw-composite.yang file is generated
  # in <root>/.build by the top-level CMakeLists.txt.
  rift_add_yang_target(
    TARGET rwcomposite_yang_parse
    YANG_FILES ${CMAKE_CURRENT_BINARY_DIR}/../../../../rw-composite.yang
    COMPONENT Composite-Yang-1.0
    DEPENDS copy_rw_composite_yang
  )

  # Workaround whatever mess rw.package is doing as it can't seem
  # to figure out that it should make a directory -before- making
  # symlinks..
  set(dir usr/lib64/python${RIFT_PYTHON3}/site-packages/gi/overrides)
  install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/.cpack-workaround
    DESTINATION ${dir})

  if (RIFT_SUPPORT_PYTHON2)
    set(dir usr/lib64/python${RIFT_PYTHON2}/site-packages/gi/overrides)

    install(FILES
      ${CMAKE_CURRENT_SOURCE_DIR}/.cpack-workaround
      DESTINATION ${dir})
  endif()

  add_custom_target(force_composite ALL DEPENDS rwcomposite_yang_parse)
endif()

##
# This macro adds targets for documentaion, unittests, code coverage and packaging
##
rift_add_submodule_targets(SUBMODULE_PACKAGE_NAME "rw.yang_composite")
