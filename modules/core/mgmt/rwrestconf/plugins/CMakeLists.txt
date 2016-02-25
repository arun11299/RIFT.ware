# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Max Beckett
# Creation Date: 8/25/2015
# 

include(rift_yang)
include(rift_python)
include(rift_plugin)


rift_install_python_plugin(
  restconf
    restconf.py
  )

rift_add_yang_target(
  TARGET
    rwrestconf_yang
  YANG_FILES
    rw-restconf.yang
  COMPONENT
    rwrestconf
  LIBRARIES
    rwmanifest_yang_gen
    rw_vcs_yang_gen
  )

rift_install_python_plugin(
  reststream
    reststream.py
  )

rift_add_yang_target(
  TARGET
    rwreststream_yang
  YANG_FILES
    rw-reststream.yang
  COMPONENT
    rwrestconf
  LIBRARIES
    rwmanifest_yang_gen
    rw_vcs_yang_gen
  )

rift_install_python_plugin(
  restportforward
    restportforward.py
  )

rift_add_yang_target(
  TARGET
    restportforward_yang
  YANG_FILES
    rw-restportforward.yang
  COMPONENT
    rwrestconf
  LIBRARIES
    rwmanifest_yang_gen
    rw_vcs_yang_gen
  )

# for testing dynamic schema
rift_add_yang_target(
  TARGET
    rwdynschematesta_yang
  YANG_FILES
    rw-dyn-schema-test-a.yang
  LIBRARIES
    rw_vcs_yang_gen
  )

rift_add_yang_target(
  TARGET
    rwdynschematestb_yang
  YANG_FILES
    rw-dyn-schema-test-b.yang
  LIBRARIES
    rw_vcs_yang_gen
  )
