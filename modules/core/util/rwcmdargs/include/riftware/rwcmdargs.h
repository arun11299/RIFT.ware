
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */



/**
 * @file rwcmdargs.h
 * @author Tom Seidenberg (tom.seidenberg@riftio.com)
 * @date 02/14/2014
 * @brief C header file for RW.CmdAgs
 */

#ifndef RIFT_RWCMDARGS_RWCMDARGS_H_
#define RIFT_RWCMDARGS_RWCMDARGS_H_

#include <sys/cdefs.h>

#include <protobuf-c/rift-protobuf-c.h>

#include "rwlib.h"

__BEGIN_DECLS

struct rwcmdargs_base_;
typedef struct rwcmdargs_base_ rwcmdargs_t; ///< anonymous typedef to hide C++ type

/**
 * Yang file list entry.
 */
typedef struct rwcmdargs_yang_entry_ {
  const char* module; ///< module name
} rwcmdargs_yang_entry_t;


typedef enum {
  RWCMDARGS_STATUS_SUCCESS = 1,            ///< Parsing successful, execute program.
  RWCMDARGS_STATUS_SUCCESS_EXTRA,          ///< Parsing successful, but extra data detected.
  RWCMDARGS_STATUS_SUCCESS_EXIT_SAVED,     ///< exit(0) requested - saved the config.
  RWCMDARGS_STATUS_SUCCESS_EXIT_VERIFIED,  ///< exit(0) requested - verified the config.
  RWCMDARGS_STATUS_SUCCESS_EXIT_CLI,       ///< exit(0) requested - ran the interactive CLI.
  RWCMDARGS_STATUS_SUCCESS_EXIT_SHOW,      ///< exit(0) requested - displayed the config.
  RWCMDARGS_STATUS_SUCCESS_EXIT_HELP,      ///< exit(0) requested - showed help.
  RWCMDARGS_STATUS_SUCCESS_EXIT_VERSION,   ///< exit(0) requested - showed version.
  RWCMDARGS_STATUS_BAD_XML,                ///< Error exit requested: XML parsing errors.
  RWCMDARGS_STATUS_BAD_VALUE,              ///< Error exit requested: bad config value(s).
  RWCMDARGS_STATUS_BAD_OPTION,             ///< Error exit requested: bad ARGV entry.
  RWCMDARGS_STATUS_BAD_ARGC,               ///< Error exit requested: ARGV too short.
  RWCMDARGS_STATUS_BAD_NODE,               ///< Error exit requested: extra data found in strict mode.
  RWCMDARGS_STATUS_BAD_PBCM,               ///< Error exit requested: unable to convert to protobuf.
} rwcmdargs_status_t;


rwcmdargs_t* rwcmdargs_alloc(bool_t want_debug);

void rwcmdargs_free(rwcmdargs_t* rwca);

rw_status_t rwcmdargs_load_yang_list(rwcmdargs_t* rwca, const rwcmdargs_yang_entry_t* yangs, size_t yangcnt);

rw_status_t rwcmdargs_load_yang(rwcmdargs_t* rwca, const char* module);

rw_status_t rwcmdargs_load_from_xml(rwcmdargs_t* rwca, const char *filename);

rw_status_t rwcmdargs_load_from_config(rwcmdargs_t* rwca, const char *filename);

rw_status_t rwcmdargs_parse_env(rwcmdargs_t* rwca);

rw_status_t rwcmdargs_parse_args(rwcmdargs_t* rwca, int* argcp, char** argv);

rw_status_t rwcmdargs_save_to_pb(rwcmdargs_t* rwca, ProtobufCMessage* pbcm);

rw_status_t rwcmdargs_interactive(rwcmdargs_t* rwca);

void rwcmdargs_interactive_quit(rwcmdargs_t* rwca);

rwcmdargs_status_t rwcmdargs_status(rwcmdargs_t* rwca);

const char* rwcmdargs_status_msg(rwcmdargs_t* rwca);

size_t rwcmdargs_status_context(rwcmdargs_t* rwca, char* buf, size_t bufsize);

__END_DECLS

#endif /* ifndef RIFT_RWCMDARGS_RWCMDARGS_H_ */
