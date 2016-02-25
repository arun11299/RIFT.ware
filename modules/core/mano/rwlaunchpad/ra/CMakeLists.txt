# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Paul Laidler
# Creation Date: 09/16/2015
# 

cmake_minimum_required(VERSION 2.8)

install(
  PROGRAMS
    pingpong_vnf_systest
    pingpong_records_systest
  DESTINATION usr/rift/systemtest/pingpong_vnf
  COMPONENT ${PKG_LONG_NAME})

install(
  FILES
    pytest/test_startstop.py
  DESTINATION usr/rift/systemtest/pytest/mission_control/launchpad
  COMPONENT ${PKG_LONG_NAME})

install(
  FILES
    pytest/conftest.py
    pytest/test_pingpong_vnf.py
    pytest/test_records.py
  DESTINATION usr/rift/systemtest/pytest/mission_control/pingpong_vnf
  COMPONENT ${PKG_LONG_NAME})

install(
  FILES
    racfg/pingpong_vnf_systest_cloudsim.racfg
    racfg/pingpong_vnf_systest_openstack.racfg
    racfg/pingpong_records_systest_openstack.racfg
  DESTINATION usr/rift/systemtest/pingpong_vnf
  COMPONENT ${PKG_LONG_NAME})

