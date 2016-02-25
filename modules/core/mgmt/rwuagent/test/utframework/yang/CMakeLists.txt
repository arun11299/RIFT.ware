# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Tom Seidenberg
# Creation Date: 2014/04/03
# 

cmake_minimum_required(VERSION 2.8)

##
# Yang targets
##
rift_add_yang_target(
  TARGET uagent_ut_test_yang
  YANG_FILES ut-types.yang interfaces.yang routes.yang dns.yang ntp.yang notif.yang vehicle-b.yang vehicle-b-augment.yang
  NO_INSTALL
)

rift_add_yang_target(
  TARGET utcomposite_yang_parse
  YANG_FILES ut-composite.yang
  NO_INSTALL
  LIBRARIES uagent_ut_test_yang_gen rwuagent_yang_gen
  DEPENDS uagent_ut_test_yang
)
