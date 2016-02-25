/* 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 * */

/**
 * @file rwutcli.hpp
 * @author Tom Seidenberg
 * @date 2014/06/04
 * @brief RWCLI for unittest purposes
 */

#include "yangcli.hpp"

/**
 * Class creating a simple CLI for unittests
 */
class RwUtCli 
: public rw_yang::BaseCli
{
 public:
  typedef rw_yang::AppDataParseToken<
    const rw_yang_pb_msgdesc_t*,
    rw_yang::AppDataTokenDeleterNull
  > pbmsg_adpt_t;

 public:
  RwUtCli(rw_yang::YangModel& model, bool debug);
  ~RwUtCli();

 public:
  // Overrides of the base class.
  bool handle_command(rw_yang::ParseLineResult* r);

 public:
  rw_status_t interactive();
  rw_status_t interactive_nb();
  void interactive_quit();


 public:
  /// Interactive editline object, created only when needed
  rw_yang::CliEditline* editline_;

  /// Enable debugging by printing extra stuff
  bool debug_;

  /// Lookup token for unit test callback extensions
  pbmsg_adpt_t pbmsg_adpt;
};

