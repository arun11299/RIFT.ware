
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */



#ifndef _RWYANGUTIL_H_
#define _RWYANGUTIL_H_

__BEGIN_DECLS

#define MAX_HOSTNAME_SZ 64

#define LOCK_FILE_NAME           ".schema.lck."
#define SCHEMA_LOCK_FILE         "./var/rift/schema/lock/LOCK_FILE_NAME"
#define SCHEMA_LOCK_DIR          "./var/rift/schema/lock"
#define SCHEMA_TMP_LOC           "./var/rift/schema/tmp/"
#define SCHEMA_VER_DIR           "./var/rift/schema/version/"
#define LATEST_VER_DIR           "./var/rift/schema/version/latest"
#define IMAGE_SCHEMA_DIR         "./usr/data/yang"
#define DYNAMIC_SCHEMA_DIR       "./var/rift/schema"
#define CONFD_WS_PREFIX          "confd_ws."
#define CONFD_PWS_PREFIX         "confd_persist_"
#define AR_CONFD_PWS_PREFIX      "ar_confd_persist_"

// The number of top level directories in DYNAMIC_SCHEMA_DIR
#define RWDYNSCHEMA_TOP_LEVEL_DIRECTORY_COUNT (8)

// Lock file liveness, in seconds
#define LIVENESS_THRESH (60)

// Version directory staleness, in seconds
#define STALENESS_THRESH (60)

// File protocol ops binary
#define FILE_PROTO_OPS_EXE ("/usr/bin/rwyangutil")

__END_DECLS

#endif
