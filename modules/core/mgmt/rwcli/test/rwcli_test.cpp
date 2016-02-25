/* 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 * */

/**
 * @file rwcli_test.cpp
 * @author Anil Gunturu (anil.gunturu@riftio.com)
 * @date 2014/03/16
 * @brief Tests for RwCLI
 */

#include <limits.h>
#include <cstdlib>
#include <iostream>
#include "../src/rwcli.hpp"
#include "rwlib.h"
#include "rwut.h"

rw_status_t
rwpython_util_print(struct rwvcs_instance_s *rwvcs,
                    const char *xml_str,
                    const char *print_format)
{
  // dummy implementation
  return RW_STATUS_SUCCESS;
}



TEST(RwCLI, PrintHook)
{
  bool status = false;

  TEST_DESCRIPTION("Test CLI print hooks");
  RwCLI *rw_cli = new RwCLI(false, nullptr);
  ((void)rw_cli);
  rw_cli->module_names_.insert("rwcli_test");
  status = rw_cli->load_manifest("../cli_test.xml");
  ASSERT_TRUE(status);

  ParseLineResult r(*rw_cli->parser, 
                    "show port counters all", 
                    ParseLineResult::NO_OPTIONS);
  ASSERT_TRUE(r.cli_print_hook_string_);

  ASSERT_THAT(r.cli_print_hook_string_,
    testing::StrEq("rwcli_plugin-python:default_print"));
}

TEST(RwCLI, NewMode)
{
  bool status = false;

  TEST_DESCRIPTION("Test modes");
  RwCLI *rw_cli = new RwCLI(false, nullptr);
  ((void)rw_cli);
  rw_cli->module_names_.insert("rwcli_test");
  status = rw_cli->load_manifest("../cli_test.xml");
  ASSERT_TRUE(status);


  ParseLineResult r(*rw_cli->parser, 
                    "port", 
                    ParseLineResult::ENTERKEY_MODE);
  ASSERT_TRUE(r.result_node_->is_mode());

  ParseLineResult q(*rw_cli->parser, "port counters",  ParseLineResult::ENTERKEY_MODE);
  ASSERT_TRUE(q.result_node_->is_mode());

 #if 0
  ParseLineResult s(*rw_cli->parser, "show",  ParseLineResult::ENTERKEY_MODE);
  ASSERT_TRUE(!s.result_node_->is_mode());
#endif  
}


