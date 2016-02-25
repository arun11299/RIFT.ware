# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Tom Seidenberg
# Creation Date: 2014/04/08
# 

set(source_yang_files
  rw-iwp.yang
  rw-launchpad-log.yang
  rw-launchpad.yang
  rw-monitor.yang
  rw-nsm.yang
  rw-resource-mgr.yang
  rw-vnfm.yang
  rw-vns.yang
  )
##
# Yang targets
##
rift_add_yang_target(
  TARGET rwlaunchpad_yang
  YANG_FILES ${source_yang_files}
  COMPONENT ${PKG_LONG_NAME}
  LIBRARIES
    mano_yang_gen
    rwcloud_yang_gen
    rw_conman_yang_gen
    rwconfig_agent_yang_gen
  DEPENDS
    mano_yang
    rwcloud_yang
    rw_conman_yang
    rwconfig_agent_yang
)

