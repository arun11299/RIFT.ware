# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

cat <<EOF >$STAGING/etc/security/limits.d/90-rift.conf
# allow core files
*   soft    core    unlimited

EOF
