
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */



/**
 * @file rwcmdargs_test.cpp
 * @author Tom Seidenberg
 * @date 02/19/2014
 * @brief Rift CmdArgs CLI test program.
 *
 * This program doesn't really do anything - this is just for playing
 * around and testing code changes.  As enhancements are made to the
 * RW.CmdArgs library, this program should be enhanced to support the
 * new features.
 */

#include <limits.h>
#include <cstdlib>
#include <iostream>

#include "rwlib.h"
#include "rwcmdargs.hpp"
#include "rwcmdargs-test.pb-c.h"
#include "rwcmdargs-test.pb-c.h"
#include "rwcmdargs_test_gen.h"


using namespace rw_cmdargs;
using namespace rw_yang;

int main(int argc, char** argv)
{
  (void)argv;
  (void)argc;
  bool want_debug = false;

  rwcmdargs_t* rwca = NULL;
  rw_status_t rs = rwcmdargs_test_gen_init(&rwca, want_debug);
  RW_ASSERT(RW_STATUS_SUCCESS == rs);
  RW_ASSERT(rwca);
  RW_ASSERT(rwcmdargs_status(rwca) == RWCMDARGS_STATUS_SUCCESS);

  CmdArgs* cmdargs = static_cast<CmdArgs*>(rwca);
  RW_ASSERT(cmdargs);
  RW_ASSERT(cmdargs->parser);
  RW_ASSERT(cmdargs->xmlmgr);
  RW_ASSERT(cmdargs->manifest);
  RW_ASSERT(cmdargs->model);
  RW_ASSERT(!cmdargs->editline);

  if (want_debug) {
    std::cout << std::endl << std::endl << "Dumping model:" << std::endl;
    rw_yang_model_dump(cmdargs->model, 2/*indent*/, 1/*verbosity*/);

    std::cout << std::endl << std::endl << "Dumping the current DOM:" << std::endl;
    cmdargs->manifest->to_stdout();
  }

  rs = cmdargs->interactive();
  if (want_debug) {
    std::cout << "CLI return status: " << rs << std::endl;
    std::cout << "CmdArgs status: " << cmdargs->status << std::endl;
  }

  RW_ASSERT(!cmdargs->editline);
  delete cmdargs;

  return 0;
}


// rwcmdargs_test_cli.cpp
