# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Tom Seidenebrg
# Creation Date: 2014/06/04
# 

set(rwutcli_base_yang_files rw-utcli-base.yang)

rift_add_yang_target(
  TARGET rwutcli_base_yang
  YANG_FILES ${rwutcli_base_yang_files}
  COMPONENT ${PKG_LONG_NAME}
  LIBRARIES
    rwext_yang_gen
    ietf_yang_gen
)

