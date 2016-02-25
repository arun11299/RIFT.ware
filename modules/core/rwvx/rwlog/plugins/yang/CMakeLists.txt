# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Austin Cormier
# Creation Date: 01/04/2015
# 

##
# Parse the yang files
##
set(source_yang_files rw-generic.yang)

rift_add_yang_target(
  TARGET rwgeneric_yang
  YANG_FILES ${source_yang_files}
  COMPONENT ${PKG_LONG_NAME}
  LIBRARIES
    rwschema_yang_gen
  )
