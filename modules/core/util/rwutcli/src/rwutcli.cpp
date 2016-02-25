/* 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 * */

/**
 * @file rwutcli.cpp
 * @author Tom Seidenberg
 * @date 2014/06/04
 * @brief RWCLI for unittest purposes
 */

#define RW_YANGPBC_ENABLE_UTCLI
#include "rwutcli.hpp"
#include <stack>

using namespace rw_yang;

/**
 * Helper function to make a fully-typed cli.
 */
RwUtCli& utcli(BaseCli& cli)
{
  return static_cast<RwUtCli&>(cli);
}

/**
 * Create a RwUtCli object.
 */
RwUtCli::RwUtCli(YangModel& model, bool debug)
: BaseCli(model),
  editline_(nullptr),
  debug_(debug),
  pbmsg_adpt(
    pbmsg_adpt_t::create_and_register(
      "http://riftio.com/ns/core/util/rwutcli/src/rwutcli.cpp",
      "pbmsg",
      this))
{
  ModeState::ptr_t new_root_mode(new ModeState(*this,*root_parse_node_));
  mode_stack_root(std::move(new_root_mode));

  model.load_module("rw-utcli-base");

  add_builtins();
  setprompt();
}

/**
 * Destroy RwUtCli object and all related objects.
 */
RwUtCli::~RwUtCli()
{
  if (editline_) {
    delete editline_;
  }
}

/**
 * Handle a command callback.  This callback will receive notification
 * of all syntactically correct command lines.  It should return false
 * if there are any errors.
 */
bool RwUtCli::handle_command(ParseLineResult* r)
{
  const rw_yang_pb_msgdesc_t* msgdata = nullptr;
  ParseNode* pn;
  if (pbmsg_adpt.parse_result_check_and_get_last(r, &pn, &msgdata)) {
    std::vector<const char*> argv;
    argv.push_back(pn->value_get().c_str());
    std::stack<std::pair<ParseNode*,ParseNode::child_citer_t>> stack;

    ParseNode::child_citer_t ci = pn->children_.cbegin();
    while (1) {
      // Pop the stack?
      if (ci == pn->children_.cend()) {
        if (stack.empty()) {
          // No more stack, done.
          break;
        }
        pn = stack.top().first;
        ci = stack.top().second;
        stack.pop();
        ++ci;
        continue;
      }

      // Save the next word to argv.
      argv.push_back((*ci)->value_get().c_str());

      // push the stack?
      if ((*ci)->children_.size()) {
        stack.emplace(pn, ci);
        pn = ci->get();
        ci = pn->children_.cbegin();
      } else {
        // no, keep going in the current children list...
        ci++;
      }
    }

    rw_status_t rs = msgdata->utcli_callback_argv(argv.size(), argv.data());
    return rs == RW_STATUS_SUCCESS;
  }

  // Look for builtin commands
  if (r->line_words_.size() > 0) {

    if (r->line_words_[0] == "exit" ) {
      RW_BCLI_ARGC_CHECK(r,1);
      mode_exit();
      return true;
    }

    if (r->line_words_[0] == "end" ) {
      RW_BCLI_ARGC_CHECK(r,1);
      mode_end_even_if_root();
      return true;
    }

    if (r->line_words_[0] == "quit" ) {
      RW_BCLI_ARGC_CHECK(r,1);
      interactive_quit();
      return true;
    }
  }

  // ATTN: Print the parse tree, to help debugging...
  std::cout << "Parsing tree:" << std::endl;
  r->parse_tree_->print_tree(4,std::cout);

  // ATTN: Print the result tree, to help debugging...
  std::cout << "Result tree:" << std::endl;
  r->result_tree_->print_tree(4,std::cout);

  std::cout << "Unrecognized command" << std::endl;
  return false;
}

/**
 * Run an interactive CLI.  Does not allow recursion - only one
 * interactive CLI can be active at the same time.  This function will
 * not return until the user exits the CLI.
 *
 * A CLI can only be started from the root mode.  If a parser wants to
 * start a new CLI (for example, when parsing the command line
 * arguments, one of the arguments requests an interactive CLI), a
 * whole new TestBaseCli object should be created and the CLI started on
 * the new object, from the root context.
 */
rw_status_t RwUtCli::interactive()
{
  if (editline_) {
    std::cerr << "CLI is already running" << std::endl;
    return RW_STATUS_FAILURE;
  }

  if (current_mode_ != root_mode_) {
    std::cerr << "Cannot start CLI outside of root mode context" << std::endl;
    return RW_STATUS_FAILURE;
  }

  editline_ = new CliEditline( *this, stdin, stdout, stderr );
  editline_->readline_loop();
  delete editline_;
  editline_ = NULL;
  mode_end_even_if_root();
  return RW_STATUS_SUCCESS;
}

/**
 * Run an interactive CLI in non-blocking mode.  Does not allow
 * recursion - only one interactive CLI can be active at the same time.
 * This function will not return until the user exits the CLI.
 *
 * A CLI can only be started from the root mode.  If a parser wants to
 * start a new CLI (for example, when parsing the command line
 * arguments, one of the arguments requests an interactive CLI), a
 * whole new TestBaseCli object should be created and the CLI started on
 * the new object, from the root context.
 */
rw_status_t RwUtCli::interactive_nb()
{
  if (editline_) {
    std::cerr << "CLI is already running" << std::endl;
    return RW_STATUS_FAILURE;
  }

  if (current_mode_ != root_mode_) {
    std::cerr << "Cannot start CLI outside of root mode context" << std::endl;
    return RW_STATUS_FAILURE;
  }

  editline_ = new CliEditline( *this, stdin, stdout, stderr );
  editline_->nonblocking_mode();

  int fd = fileno(stdin);
  fd_set fds = {0};
  editline_->read_ready();  // Yes, gratuitous read_ready is needed

  while (!editline_->should_quit()) {
    editline_->read_ready();
    FD_SET(fd, &fds);
    int nfds = select( fd+1, &fds, NULL, NULL, NULL );
    if (nfds == 1) {
      RW_ASSERT(FD_ISSET(fd, &fds));
    } else if(nfds == -1) {
      switch (errno) {
        case EINTR:
          continue;
        case EBADF:
        case EINVAL:
        default:
          std::cerr << "Unexpected errno " << errno << std::endl;
          editline_->quit();
      }
    } else {
      RW_ASSERT_NOT_REACHED(); // nfds == 0?
    }
  }

  delete editline_;
  editline_ = NULL;
  mode_end_even_if_root();
  return RW_STATUS_SUCCESS;
}

/**
 * Quit the interactive CLI.
 */
void RwUtCli::interactive_quit()
{
  if (editline_) {
    editline_->quit();
  }
}

/**
 * Load a command list generated by yangpbc.
 */
// void RwUtCli::add_commands(const rw_yang_pb_msgdesc_t* const* msgs)
// {
//   RW_ASSERT(msgs);
//   while (*msgs) {
//     const rw_yang_pb_msgdesc_t* ypbc_msgdesc = *msgs;
//     RW_ASSERT(ypbc_msgdesc);
//     YangNode* ynode = model_.search_node_path(ypbc_msgdesc->path);
//     RW_ASSERT(ynode); // assert because this is just unit test code - fix your module loads

//     const rw_yang_pb_msgdesc_t* old_ypbc_msgdesc
//       = ynode->app_data_set_and_keep_ownership(pbmsg_adpt.get_yang_token(), ypbc_msgdesc);
//     RW_ASSERT(!old_ypbc_msgdesc);
//     ++msgs;
//   }
// }
