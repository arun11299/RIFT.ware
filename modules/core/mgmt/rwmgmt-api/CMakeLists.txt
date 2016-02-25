# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Austin Cormier
# Creation Date: 11/11/2014
# 

install(DIRECTORY rwmgmtapi DESTINATION usr/${RIFT_PYDIR} COMPONENT RWMGMT)

set(MGMT_BIN_FILES
  bin/rwmgmt-api-standalone)

install(FILES ${MGMT_BIN_FILES}
        DESTINATION usr/local/bin
        COMPONENT RWMGMT
        PERMISSIONS OWNER_READ OWNER_EXECUTE
                    GROUP_READ GROUP_EXECUTE
                    WORLD_READ WORLD_EXECUTE)

file(GLOB MGMT_ETC_FILES etc/*.*)
install(FILES ${MGMT_ETC_FILES} DESTINATION etc COMPONENT RWMGMT)

rift_pytest(test_confd
  TEST_ARGS rwmgmtapi/test/test_confd.py
  TEST_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

rift_pytest(test_rwxml
  TEST_ARGS rwmgmtapi/test/test_rwxml.py
  TEST_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

rift_pytest(test_xml2json
  TEST_ARGS rwmgmtapi/test/test_xml2json.py
  TEST_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

rift_pytest(test_vcs
  TEST_ARGS rwmgmtapi/vcs/test/test_builder.py
  TEST_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

rift_pytest(test_vnf
  TEST_ARGS rwmgmtapi/vnf/test/test_vnf.py
  TEST_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

rift_pytest(test_fpath
  TEST_ARGS rwmgmtapi/fpath/test/test_fpath.py
  TEST_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
