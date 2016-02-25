/* 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 * */

/**
 * @file rwutcli.hpp
 * @author Tom Seidenberg
 * @date 2014/06/04
 * @brief Tests for rwut cli
 */

#if !defined(RW_YANGPBC_ENABLE_UTCLI)
#error "Must compile with -DRW_YANGPBC_ENABLE_UTCLI"
#endif

#include "rwutcli.hpp"
#include "yangncx.hpp"
#include "test_rwutcli.pb-c.h"

using namespace rw_yang;

int main(int argc, char **argv)
{
  bool want_debug = true;

  YangModel::ptr_t model(std::move(YangModelNcx::create_model()));
  model->load_module("test_rwutcli");

  std::unique_ptr<RwUtCli> utcli(new RwUtCli(*model.get(), want_debug));

  //  utcli->add_commands(RWPB_G_SCHEMA_YPBCSD(TestRwutcli)->top_msglist);

  utcli->interactive();

  utcli.reset();
  model.reset();

  exit(0);
}
