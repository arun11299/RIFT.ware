# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Creation Date: 11/26/2015
# 

cmake_minimum_required(VERSION 2.8)

##
# Submodule specific includes will go here,
# These are specified here, since these variables are accessed
# from multiple sub directories. If the variable is subdirectory
# specific it must be declared in the subdirectory.
##
set(PKG_NAME rwuagent)
set(PKG_VERSION 1.0)
set(PKG_RELEASE 1)
set(PKG_LONG_NAME ${PKG_NAME}-${PKG_VERSION})

##
# Rift wrapper/config files
##
install(PROGRAMS rw_confd
        DESTINATION usr/bin
        COMPONENT ${PKG_LONG_NAME}
       )

install(FILES rw_confd.conf
        DESTINATION etc
        COMPONENT ${PKG_LONG_NAME}
       )
