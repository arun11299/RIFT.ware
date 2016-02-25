
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */



/**
 * @file rwcmdargs.cpp
 * @author Tom Seidenberg (tom.seidenberg@riftio.com)
 * @date 02/12/2014
 * @brief RW.CmdArgs - command arguments parsing
 */

#include <vector>
#include <string>
#include <string.h>
#include <utility>

#include "yangcli.hpp"
#include "yangncx.hpp"
#include "rwcmdargs.hpp"


using namespace rw_yang;
using namespace rw_cmdargs;

/**
 * Allocate a CmdArgs and populate with the basic CLI.  This API exists
 * to return the CmdArgs object as an anonymous struct to C code.
 */
rwcmdargs_t* rwcmdargs_alloc(bool_t want_debug)
{
  return CmdArgs::rwcmdargs_alloc(want_debug);
}

/**
 * Free a CmdArgs and release any resources it held.
 */
void rwcmdargs_free(rwcmdargs_t* rwca)
{
  delete static_cast<CmdArgs*>(rwca);
}

/**
 * Load a list of yang files into the parser to extend the syntax.
 * This API should generally be used only by automatic code generator.
 * Explicit callers should use the class object directly.
 */
rw_status_t rwcmdargs_load_yang_list(rwcmdargs_t* rwca, const rwcmdargs_yang_entry_t* yangs, size_t yangcnt)
{
  return static_cast<CmdArgs*>(rwca)->load_yang_files(yangs,yangcnt);
}

/**
 * Load a list of yang files into the parser to extend the syntax.
 * This API should generally be used only by automatic code generator.
 * Explicit callers should use the class object directly.
 */
rw_status_t rwcmdargs_load_yang(rwcmdargs_t* rwca, const char* module)
{
  return static_cast<CmdArgs*>(rwca)->load_yang_file(module);
}

/**
 * Load an XML-format manifest file explicitly.
 */
rw_status_t rwcmdargs_load_from_xml(rwcmdargs_t* rwca, const char *filename)
{
  return static_cast<CmdArgs*>(rwca)->load_from_xml(filename);
}

/**
 * Load a CLI-format manifest file explicitly.
 */
rw_status_t rwcmdargs_load_from_config(rwcmdargs_t* rwca, const char *filename)
{
  return static_cast<CmdArgs*>(rwca)->load_from_config(filename);
}

/**
 * Parse the Linux shell environment for known RW.CmdArgs variables.
 */
rw_status_t rwcmdargs_parse_env(rwcmdargs_t* rwca)
{
  return static_cast<CmdArgs*>(rwca)->parse_env();
}

/**
 * Parse the command line arguments.
 */
rw_status_t rwcmdargs_parse_args(rwcmdargs_t* rwca, int* argcp, char** argv)
{
  return static_cast<CmdArgs*>(rwca)->parse_args(argcp,argv);
}

/**
 * Parse the command line arguments.
 */
rw_status_t rwcmdargs_save_to_pb(rwcmdargs_t* rwca, ProtobufCMessage* pbcm)
{
  return static_cast<CmdArgs*>(rwca)->save_to_pb(pbcm);
}

/**
 * Run the interactive CLI.
 */
rw_status_t rwcmdargs_interactive(rwcmdargs_t* rwca)
{
  return static_cast<CmdArgs*>(rwca)->interactive();
}

/**
 * Quit the interactive CLI.
 */
void rwcmdargs_interactive_quit(rwcmdargs_t* rwca)
{
  static_cast<CmdArgs*>(rwca)->interactive_quit();
}

/**
 * Get the current parser status.  Exit successes and errors are sticky
 * until the next interactive CLI.
 */
rwcmdargs_status_t rwcmdargs_status(rwcmdargs_t* rwca)
{
  return static_cast<CmdArgs*>(rwca)->status;
}

/**
 * If the parser is in an error state (STATUS_BAD), then this API will
 * return a pointer to a const, static, NUL-terminated description
 * string for the state.
 */
const char* rwcmdargs_status_msg(rwcmdargs_t* rwca)
{
  UNUSED(rwca);
  // ATTN
  return "";
}

/**
 * Fill in a string buffer with the context of the current error state.
 * Does nothing if not in an error state.  Returns the number of
 * characters placed into the buffer, not including the terminating
 * NUL.  Will always NUL terminate the buffer.
 */
size_t rwcmdargs_status_context(rwcmdargs_t* rwca, char* buf, size_t bufsize)
{
  void* eoc = memccpy(buf, static_cast<CmdArgs*>(rwca)->status_context.c_str(), '\0', bufsize);
  if (!eoc) {
    buf[bufsize-1] = '\0';
    return bufsize-1;
  }
  return (char*)eoc - &buf[0] - 1;
}



namespace rw_cmdargs {

using namespace rw_yang;

CmdArgsModeState::CmdArgsModeState(
  CmdArgs& cmdargs,
  CmdArgsParser& parser,
  const ParseNode& root_node)
: ModeState(parser, root_node),
  cmdargs(cmdargs)
{}

#if 0
CmdArgsModeState::CmdArgsModeState(
    CmdArgs& cmdargs,
    xmlNode* domnode,
    ParseLineResult* r)
: ModeState(r),
  cmdargs(cmdargs),
  domnode(domnode)
{}

CmdArgsModeState::CmdArgsModeState(
    CmdArgs& cmdargs,
    xmlNode* domnode,
    ParseNode::ptr_t&& result_tree,
    ParseNode* result_node)
: ModeState(std::move(result_tree), result_node),
  cmdargs(cmdargs),
  domnode(domnode)
{}
#endif

/**
 * Constructor for the CmdArgs CLI core.
 */
CmdArgsParser::CmdArgsParser(YangModel& model, CmdArgs& owner)
: BaseCli(model),
  cmdargs(owner)
{
  ModeState::ptr_t new_root(new CmdArgsModeState(owner, *this, *root_parse_node_));
  mode_stack_root(std::move(new_root));
}

/**
 * Handle the base-CLI command callback.  This callback will receive
 * notification of all syntactically correct command lines.  It should
 * return false if there are any errors.
 */
bool CmdArgsParser::handle_command(ParseLineResult* r)
{
  return cmdargs.handle_command(r);
}

/**
 * Handle the base-CLI mode_enter callback.  Dispatch to the owning
 * CmdArgs object.
 */
void CmdArgsParser::mode_enter(ParseLineResult* r)
{
  cmdargs.mode_enter(r);
}

/**
 * Handle the base-CLI mode_enter callback.  Dispatch to the owning
 * CmdArgs object.
 */
void CmdArgsParser::mode_enter(ParseNode::ptr_t&& result_tree,
                            ParseNode* result_node)
{
  cmdargs.mode_enter(std::move(result_tree),result_node);
}



/**
 * Create a CmdArgs object.  The created object is configured only for
 * the basic CLI.
 */
CmdArgs* CmdArgs::rwcmdargs_alloc(bool want_debug)
{
  CmdArgs* cmdargs = new CmdArgs(want_debug);
  return cmdargs;
}

/**
 * Create a CmdArgs object.  Creates the syntax model, loads the basic
 * RW.CmdArgs syntax, initializes the CLI parser, and creates an empty
 * configuration.
 */
CmdArgs::CmdArgs(bool want_debug)
: parent_cmdargs(nullptr),
  xmlmgr(nullptr),
  manifest(nullptr),
  parser(nullptr),
  editline(nullptr),
  model(nullptr),
  status(RWCMDARGS_STATUS_SUCCESS),
  debug(want_debug)
{
  // Create the syntax model. The other code assumes this exists.
  model = YangModelNcx::create_model();
  RW_ASSERT(model);

  // Load the basic CLI syntax
  YangModule* module = model->load_module(CMDARGS_ROOT_MODULE);
  RW_ASSERT(module);

  // Create a CmdArgsParser instance
  parser = new CmdArgsParser( *model, *this );
  RW_ASSERT(parser);
  parser->add_builtins();
  parser->setprompt();
  parser->add_behaviorals();

  // Create the empty manifest
  xmlmgr = xml_manager_create_xerces(model).release();
  RW_ASSERT(xmlmgr);
  manifest = xmlmgr->create_document().release();
  RW_ASSERT(manifest);
  parser->root_mode_->set_xml_node(manifest->get_root_node());
}

/**
 * Create a child CmdArgs object, which begins parsing from the root
 * context.  It uses the same syntax model and configuration as the
 * parent object.
 */
CmdArgs::CmdArgs(CmdArgs* other)
: parent_cmdargs(other),
  xmlmgr(other->xmlmgr),
  manifest(other->manifest),
  parser(nullptr),
  editline(other->editline),
  model(other->model),
  status(RWCMDARGS_STATUS_SUCCESS),
  debug(other->debug)
{
  RW_ASSERT(other);
  RW_ASSERT(other->model);
  RW_ASSERT(other->parser);
  RW_ASSERT(other->parser->root_mode_);

  // Create a CmdArgsParser instance
  parser = new CmdArgsParser( *model, *this );
  RW_ASSERT(parser);
  parser->add_builtins();
  parser->setprompt();
  parser->add_behaviorals();

  // ATTN: Wants to share the config...
}

/**
 * Destroy CmdArgs object and all related objects.
 */
CmdArgs::~CmdArgs()
{
  if (parent_cmdargs) {
    // Don't free anything owned by the parent
    if (parent_cmdargs->editline == editline) {
      editline = nullptr;
    }
    if (parent_cmdargs->xmlmgr == xmlmgr) {
      xmlmgr = nullptr;
    }
    if (parent_cmdargs->manifest == manifest) {
      manifest = nullptr;
    }
    if (parent_cmdargs->model == model) {
      model = nullptr;
    }
    // ATTN: Transfer status and context?
  }

  if (editline) {
    delete editline;
  }
  if (parser) {
    delete parser;
  }
  if (manifest) {
    manifest->obj_release();
  }
  if (xmlmgr) {
    xmlmgr->obj_release();
  }
  if (model) {
    delete model;
  }
}

rw_status_t CmdArgs::load_from_xml(const char *filename)
{
  RW_ASSERT(filename);

  RW_ASSERT(xmlmgr);
  XMLDocument::uptr_t new_dom = std::move(xmlmgr->create_document_from_file(filename, true/*validate*/));
  if (!new_dom || !new_dom->get_root_node()) {
    // ATTN: complain about the error!
    return RW_STATUS_FAILURE;
  }

  // Try to splice the new document into the current configuration.
  // ATTN: Would be nice if the location of the loaded file could be
  //       relative to the current mode stack.  In order to do that,
  //       the load commands would have to be inserted into the model
  //       at "namespace-change" locations, I think (anything other
  //       than that would be harder to accomplish).
  RW_ASSERT(manifest);
  XMLNode* my_root = manifest->get_root_node();
  RW_ASSERT(my_root);
  XMLNode* other_root = new_dom->get_root_node();
  RW_ASSERT(other_root);
  YangNode* other_ynode = other_root->get_descend_yang_node();

  rw_yang_netconf_op_status_t ncs = RW_YANG_NETCONF_OP_STATUS_FAILED;

  // Check to see if the input XML was understood in the model.  Allow
  // it to have either a dummy root or a top-level root.
  if (other_ynode) {
    // See if it is a top-level in the model
    RW_ASSERT(model);
    YangNode* yroot = model->get_root_node();
    RW_ASSERT(yroot);
    if (other_ynode == yroot) {
      ncs = my_root->merge(other_root);
    } else {
      other_root->set_reroot_yang_node(other_ynode);
      XMLNode* merge_point = my_root->find(other_ynode->get_name(),other_ynode->get_ns());
      if (merge_point) {
        ncs = merge_point->merge(other_root);
      } else {
        XMLNode* appended = my_root->import_child(other_root, nullptr, true/*deep*/);
        if (appended) {
          ncs = RW_YANG_NETCONF_OP_STATUS_OK;
        }
      }
    }
  } else {
    ncs = my_root->merge(other_root);
  }

  if (RW_YANG_NETCONF_OP_STATUS_OK != ncs) {
    status = RWCMDARGS_STATUS_BAD_XML;
  }
  return rw_yang_netconf_op_status_to_rw_status(ncs);
}

rw_status_t CmdArgs::load_from_config(const char *filename)
{
#if 1
  UNUSED(filename);
  return RW_STATUS_FAILURE;
#else
  std::ifstream infile(filename);
  std::string line;
  int num_lines = 0;
  clock_t begin = clock();
  rw_status_t status = RW_STATUS_SUCCESS;

  while (std::getline(infile, line)) {
    bool success;
    std::cout << line << std::endl;
    success = libedit_enterkey(this, line, false);
    if (!success) {
      std::cout << "Load config terminated" << std::endl;
      std::cout << "Failed line: " << line << std::endl;
      return RW_STATUS_FAILURE;
    }
    num_lines++;
  }

  clock_t end = clock();
  double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;

  std::cout << "Load config file successful" << std::endl;
  std::
      cout << "Num lines: " << num_lines << " Elapsed time: " << elapsed_secs <<
      " sec" << std::endl;

  return status;
#endif /* 1 */
}

rw_status_t CmdArgs::save_to_xml(const char *filename)
{
  return manifest->to_file(filename);
}

rw_status_t CmdArgs::save_to_config(const char *filename)
{
  UNUSED(filename);
  return RW_STATUS_FAILURE;
}

/**
 * Try to dump the current DOM into a protobuf.
 */
rw_status_t CmdArgs::save_to_pb(ProtobufCMessage* pbcm)
{
  RW_ASSERT(pbcm);
  RW_ASSERT(manifest);

  // ATTN: need to capture errors
  rw_yang_netconf_op_status_t ncs = manifest->to_pbcm(pbcm);

  rw_status_t rs = RW_STATUS_SUCCESS;
  if (ncs != RW_YANG_NETCONF_OP_STATUS_OK) {
    status = RWCMDARGS_STATUS_BAD_PBCM;
    rs = RW_STATUS_FAILURE;
  }
  return rs;
}

void CmdArgs::show_config_text()
{
#if 0
  clock_t begin = clock();

  int mindex = 0;
  bool begin_line = true;
  show_config_text(&config.parse_node, &mindex, &begin_line);

  clock_t end = clock();
  double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;

  std::cout << "Show config successful" << std::endl;
  std::cout << "Elapsed time: " << elapsed_secs << " sec" << std::endl;
#endif /* 0 */
}

void CmdArgs::show_config_text(ParseNode *parse_node,
                                int *mindex,
                                bool *begin_line)
{
  UNUSED(parse_node);
  UNUSED(mindex);
  UNUSED(begin_line);
  // ATTN: Need to implement.  How does RwCLI do it?
}

void CmdArgs::show_config_xml()
{
  RW_ASSERT(manifest);
  manifest->to_stdout();
  std::cout << std::endl;
}

/**
 * Load a single yang module.  This API is typically called from
 * generated code, although you could call it directly.
 */
rw_status_t CmdArgs::load_yang_file(const char *module)
{
  RW_ASSERT(model);
  YangModule* ymod = model->load_module(module);
  if (ymod) {
    return RW_STATUS_SUCCESS;
  }
  return RW_STATUS_FAILURE;
}

/**
 * Load a list of yang module.  This API is typically called from
 * generated code, although you could call it directly.
 */
rw_status_t CmdArgs::load_yang_files(const rwcmdargs_yang_entry_t* yangs, size_t yangcnt)
{
  RW_ASSERT(yangs);
  rw_status_t rs = RW_STATUS_SUCCESS;
  for( size_t i=0; i<yangcnt; ++i ) {
    RW_ASSERT(yangs[i].module);
    YangModule* ymod = model->load_module(yangs[i].module);
    if (nullptr == ymod) {
      rs = RW_STATUS_FAILURE;
      // but keep going
    }
  }
  return rs;
}

rw_status_t CmdArgs::load_yang_buf(const char* buf, size_t bufsize, const char* desc)
{
  UNUSED(buf);
  UNUSED(bufsize);
  UNUSED(desc);
  return RW_STATUS_FAILURE;
}

rw_status_t CmdArgs::parse_env()
{
  RW_ASSERT(parser);
  // ATTN: std::cerr << "parse_env() not yet supported" << std::endl;
  return RW_STATUS_SUCCESS;
}

/**
 * Parse the arguments.
 */
rw_status_t CmdArgs::parse_args(int argc, char** argv)
{
  RW_ASSERT(parser);
  RW_ASSERT(argc); // should be at least the program name.
  RW_ASSERT(argv);

  int a = 1; // current phrase start
  while (a < argc) {
    std::vector<std::string> argv_words;
    for (int i = a; i<argc; ++i) {
      argv_words.push_back(argv[i]);
    }

    unsigned ate = parser->parse_argv( argv_words );
    if (ate == 0) {
      std::cerr << "Unexpected argument #" << a << " '" << argv[a] << "'" << std::endl;
      return RW_STATUS_FAILURE;
    }

    a += ate;
  }
  return RW_STATUS_SUCCESS;
}

/**
 * Parse the arguments.  Consume the arguments that are recognized.
 */
rw_status_t CmdArgs::parse_args(int* argcp, char** argv)
{
  RW_ASSERT(parser);
  RW_ASSERT(argcp);
  RW_ASSERT(*argcp); // should be at least the program name.
  RW_ASSERT(argv);

  int keep = 1; // kept argc - move kept argv items to this index
  int a = 1; // current phrase start
  while (a < *argcp) {
    std::vector<std::string> argv_words;
    for (int i = a; i<*argcp; ++i) {
      argv_words.push_back(argv[i]);
    }

    unsigned ate = parser->parse_argv( argv_words );
    if (ate == 0) {
      argv[keep] = argv[a];
      ++keep;
      ++a;
      continue;
    }

    a += ate;
  }

  // Ate nothing?
  if (keep == *argcp) {
    return RW_STATUS_FAILURE;
  }

  RW_ASSERT(keep <= *argcp);
  *argcp = keep;
  return RW_STATUS_SUCCESS;
}

/**
 * Run an interactive CLI.  Does not allow recursion - only one
 * interactive CLI can be active at the same time.  This function will
 * not return until the user exits the CLI.
 *
 * A CLI can only be started from the root mode.  If a parser wants to
 * start a new CLI (for example, when parsing the command line
 * arguments, one of the arguments requests an interactive CLI), a
 * whole new CmdArgs object should be created and the CLI started on
 * the new object, from the root context.
 */
rw_status_t CmdArgs::interactive()
{
  if (editline) {
    std::cerr << "CLI is already running" << std::endl;
    return RW_STATUS_FAILURE;
  }

  if (parser->current_mode_ != parser->root_mode_) {
    std::cerr << "Cannot start CLI outside of root mode context" << std::endl;
    return RW_STATUS_FAILURE;
  }

  RW_ASSERT(parser);
  editline = new CliEditline( *parser, stdin, stdout, stderr );
  editline->readline_loop();
  delete editline;
  editline = nullptr;
  parser->mode_end_even_if_root();
  return RW_STATUS_SUCCESS;
}

/**
 * Quit the interactive CLI.
 */
void CmdArgs::interactive_quit()
{
  // ATTN: Should really ASSERT here, once cmdargs yang extensions are integrated
  if (editline) {
    editline->quit();
  }
}

rwcmdargs_status_t CmdArgs::get_status()
{
  return RWCMDARGS_STATUS_BAD_VALUE;
}

const char* CmdArgs::get_status_msg()
{
  return "";
}

void CmdArgs::get_status_context(char* buf, size_t bufsize)
{
  UNUSED(buf);
  UNUSED(bufsize);
}

bool CmdArgs::merge_command_dom(ParseLineResult* r)
{
  XMLDocument::uptr_t command = std::move(xmlmgr->create_document());
  XMLNode *parent_xml = command->get_root_node();
  ModeState::stack_iter_t iter = parser->mode_stack_.begin();
  iter++;

  while (parser->mode_stack_.end() != iter) {
    parent_xml = iter->get()->create_xml_node (command.get(), parent_xml);
    iter++;
  }

  for (auto ci = r->result_tree_->children_.begin(); ci != r->result_tree_->children_.end(); ++ci ) {
    ParseNode* pn = ci->get();
    XMLNode* result = iter->get()->merge_xml (command.get(), parent_xml, pn);
    if (!result) {
      return false;
    }
  }

  manifest->merge(command.get());
  return true;
}


/**
 * Handle the base-CLI command callback.  This callback will receive
 * notification of all syntactically correct command lines.  It should
 * return false if there are any errors.
 */
bool CmdArgs::handle_command(ParseLineResult* r)
{
  /*
   * ATTN: this code looks dumb.  Shouldn't this be auto-generated?
   * This code is just a duplication of the yang syntax tree...  There
   * ought to be yang annotations that indicate which function to call.
   * Something like:
   *
   *   rift:cli-function "function_name"
   *
   * But that implies plugins, which I don't want to support in
   * RW.CmdArgs (who bootstraps the bootstrapper?).  Maybe CmdArgs has
   * its own, very-limited function dispatcher syntax, that only works
   * for cmdargs-cli.yang.
   */

  // Look for builtin commands
  if (r->line_words_.size() > 0) {

    if (r->line_words_[0] == "exit" ) {
      RW_BCLI_ARGC_CHECK(r,1);
      RW_ASSERT(parser);
      parser->mode_exit();
      return true;
    }

    if (r->line_words_[0] == "end" ) {
      RW_BCLI_ARGC_CHECK(r,1);
      RW_ASSERT(parser);
      parser->mode_end_even_if_root();
      return true;
    }

    if (r->line_words_[0] == "quit" ) {
      RW_BCLI_ARGC_CHECK(r,1);
      interactive_quit();
      return true;
    }

    if (r->line_words_[0] == "start-cli" ) {
      RW_BCLI_ARGC_CHECK(r,1);
      rw_status_t rs = interactive();
      RW_BCLI_RW_STATUS_CHECK(rs);
      return true;
    }

    if (r->line_words_[0] == "load" ) {
      RW_BCLI_MIN_ARGC_CHECK(r,2);

      if (r->line_words_[1] == "manifest" ) {
        RW_BCLI_MIN_ARGC_CHECK(r,3);
        if (r->line_words_[2] == "file" ) {
          RW_BCLI_ARGC_CHECK(r,4);
          rw_status_t rs = load_from_xml(r->line_words_[3].c_str());
          RW_BCLI_RW_STATUS_CHECK(rs);
          return true;
        }
      }
    }
    if (r->line_words_[0] == "help" ) {
      RW_BCLI_MIN_ARGC_CHECK(r,1);
      if (r->line_words_[1] == "command" ) {
        RW_BCLI_MIN_ARGC_CHECK(r,3);
        rw_yang_node_t*node =  rw_yang_find_node_in_model(this->model,
                                                          r->line_words_[2].c_str());
        if (node)
          rw_yang_node_dump_tree(node, 2, 1);
      }else{
        rw_yang_model_dump(this->model, 2/*indent*/, 1/*verbosity*/);
      }
      return true;
    }

    if (r->line_words_[0] == "show" ) {
      RW_BCLI_MIN_ARGC_CHECK(r,2);

      if (r->line_words_[1] == "config" ) {
        RW_BCLI_MIN_ARGC_CHECK(r,3);
        if (r->line_words_[2] == "manifest" ) {
          RW_BCLI_ARGC_CHECK(r,3);
          show_config_xml();
          return true;
        }
        if (r->line_words_[2] == "config" ) {
          RW_BCLI_ARGC_CHECK(r,3);
          show_config_text();
          return true;
        }
      }
    }
  }

  bool was_data = merge_command_dom(r);
  if (was_data) {
    return true;
  }

  // ATTN: Print the parse tree, to help debugging...
  std::cout << "Unrecognized command" << std::endl;
  std::cout << "Parsing tree:" << std::endl;
  r->parse_tree_->print_tree(4,std::cout);
  return false;
}

/**
 * Handle the base-CLI mode_enter callback.  Dispatch to the owning
 * CmdArgs object.
 */
void CmdArgs::mode_enter(ParseLineResult* r)
{
  UNUSED(r);
}

/**
 * Handle the base-CLI mode_enter callback.  Dispatch to the owning
 * CmdArgs object.
 */
void CmdArgs::mode_enter(ParseNode::ptr_t&& result_tree,
                         ParseNode* result_node)
{
  UNUSED(result_tree);
  UNUSED(result_node);
}



} /* namespace rw_cmdargs */

// rwcmdargs.cpp
