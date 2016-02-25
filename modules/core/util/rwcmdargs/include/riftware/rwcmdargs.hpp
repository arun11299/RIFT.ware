
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */



/**
 * @file rwcmdargs.hpp
 * @author Tom Seidenberg (tom.seidenberg@riftio.com)
 * @date 02/12/2014
 * @brief Top level include for RW.CmdArgs
 */

#ifndef RIFT_RWCMDARGS_RWCMDARGS_HPP_
#define RIFT_RWCMDARGS_RWCMDARGS_HPP_

#if __cplusplus < 201103L
#error "Requires C++11"
#endif

#include "yangncx.hpp"
#include "yangcli.hpp"
#include "rwcmdargs.h"

/**
 * Empty base class - used for the C code APIs.  Allow the C code to
 * refer to a CmdArgs in a type-safe manner.
 */
struct rwcmdargs_base_ {};


namespace rw_cmdargs {

using namespace rw_yang;

#define CMDARGS_ROOT_MODULE "rwcmdargs-cli"
#define CMDARGS_ROOT_NAMESPACE "http://riftio.com/ns/riftware-1.0/rwcmdargs-cli";

// Forward decl
class CmdArgs;
class CmdArgsParser;


/**
 * Mode stack state.  These data structures keep track of which mode
 * the parser is in and where in the schema the modes are rooted.
 */
class CmdArgsModeState: public ModeState {
 public:
  CmdArgsModeState(CmdArgs& cmdargs,
                   CmdArgsParser& cli,
                   const ParseNode& root_node);
#if 0
  CmdArgsModeState(CmdArgs& cmdargs,
                   rw_xml::xmlNode* domnode,
                   ParseLineResult* r);
  CmdArgsModeState(CmdArgs& cmdargs,
                   rw_xml::xmlNode* domnode,
                   ParseNode::ptr_t&& result_tree,
                   ParseNode* result_node);
#endif

 public:
  CmdArgs& cmdargs;             ///< The owning cmdargs instance.
};

/**
 * RW.CmdArgs CLI parser instance.  May be more than one in memory at
 * the same time, if recursively parsing multiple files.
 */
class CmdArgsParser: public BaseCli {

 public:
  CmdArgsParser(YangModel& model, CmdArgs& cmdargs);

 public:
  // Overrides of the base class.
  bool handle_command(ParseLineResult* r);
  void mode_enter(ParseLineResult* r);
  void mode_enter(ParseNode::ptr_t&& result_tree,
                  ParseNode* result_node);

 public:
  CmdArgs& cmdargs;
};


/**
 * Empty base class - used for the C code APIs.  Allow the C code to
 * refer to a CmdArgs in a type-safe manner.
 */
class CmdArgs: public rwcmdargs_base_ {

 public:
  static CmdArgs* rwcmdargs_alloc(bool want_debug);

 public:
  CmdArgs(bool want_debug);
  explicit CmdArgs(CmdArgs* other);
  ~CmdArgs();

 public:
  //rw_status_t load_from_mem_xml(const char* buf, size_t bufsize, const char* desc);
  //rw_status_t load_from_mem_config(const char* buf, size_t bufsize, const char* desc);
  //rw_status_t load_from_dom(domdocument);
  //rw_status_t load_from_pb(const char *filename);
  rw_status_t load_from_xml(const char *filename);
  rw_status_t load_from_config(const char *filename);

  rw_status_t save_to_xml(const char *filename);
  rw_status_t save_to_config(const char *filename);
  rw_status_t save_to_pb(ProtobufCMessage* pbcm);
  //rw_status_t save_to_dom(domdocument);

  void show_config_text();
  void show_config_text(ParseNode *parse_node, int *mindex, bool *begin_line);
  void show_config_xml();

  rw_status_t load_yang_file(const char *yang);
  rw_status_t load_yang_files(const rwcmdargs_yang_entry_t* yangs, size_t yangcnt);
  rw_status_t load_yang_buf(const char* buf, size_t bufsize, const char* desc);

  void parse_start();
  rw_status_t parse_env();
  rw_status_t parse_args(int argc, char** argv);
  rw_status_t parse_args(int* argcp, char** argv);

  rw_status_t interactive();
  void interactive_quit();

  rwcmdargs_status_t get_status();
  const char* get_status_msg();
  void get_status_context(char* buf, size_t bufsize);

  bool merge_command_dom(ParseLineResult* r);

 public:
  // Callbacks from the CmdArgsParser object
  bool handle_command(ParseLineResult* r);

  void mode_enter(ParseLineResult* r);
  void mode_enter(ParseNode::ptr_t&& result_tree,
                  ParseNode* result_node);

 public:
  /// Parent CmdArgs instance, if parsing recursively.
  CmdArgs* parent_cmdargs;

  /// XML Manager
  XMLManager* xmlmgr;

  /// Current manifest DOM
  XMLDocument* manifest;

  /// CLI parser
  CmdArgsParser* parser;

  /// Interactive editline object, created only when needed
  CliEditline* editline;

  /// The CLI's libncx-based syntax model
  YangModel* model;

  /// Current RW.CmdArgs status
  rwcmdargs_status_t status;

  /// Context for error status
  std::string status_context;

  /// Enable debugging by printing extra stuff
  bool debug;
};


} /* namespace rw_cmdargs */

#endif // ifndef RIFT_RWCMDARGS_RWCMDARGS_HPP_
