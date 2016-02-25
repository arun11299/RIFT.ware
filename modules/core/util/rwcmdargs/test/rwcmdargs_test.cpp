
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */



/**
 * @file rwcmdargs_test.cpp
 * @author Tom Seidenberg
 * @date 02/19/2014
 * @brief RW.CmdArgs unittest program.
 *
 * Unit test program for RW.CmdArgs library.
 */

#include <limits.h>
#include <cstdlib>
#include <iostream>
#include "gmock/gmock.h"
#include "gtest/rw_gtest.h"

#include "rwlib.h"
#include "rwcmdargs.hpp"
#include "rwcmdargs-test.pb-c.h"
#include "rwcmdargs-test.pb-c.h"
#include "rwcmdargs_test_gen.h"


using namespace rw_cmdargs;
using namespace rw_yang;

char **g_argv;
int g_argc;

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc,argv);
  if( argc < 2 ) {
    std::cerr << "Expected xml file list" << std::endl;
    exit(1);
  }

  g_argv = argv;
  g_argc = argc;

  return RUN_ALL_TESTS();
}


/**
 * Verify the contents of the test XML file's message conversion.  This
 * is used in several tests.
 */
static void verify_xml(const RWPB_T_MSG(RwcmdargsTest_RwcaTestCont)& m)
{
  EXPECT_FALSE(m.has_num2);
  EXPECT_TRUE(m.str2);
  EXPECT_STREQ(m.str2,"string 2");

  EXPECT_TRUE(m.cont_in_group);
  if(m.cont_in_group) {
    EXPECT_EQ(m.cont_in_group->num1, 1234);
    EXPECT_STREQ(m.cont_in_group->str1,"string 1");
  }
}


/**
 * Make sure that you can create and delete a CmdArgs object.
 */
TEST(RWCmdArgs, CreateDestroy) {
  TEST_DESCRIPTION("CmdArgs create and destroy");
  bool want_debug = false;

  CmdArgs* cmdargs = new CmdArgs(want_debug);
  ASSERT_TRUE(cmdargs);
  EXPECT_TRUE(cmdargs->parser);
  EXPECT_TRUE(cmdargs->xmlmgr);
  EXPECT_TRUE(cmdargs->manifest);
  EXPECT_TRUE(cmdargs->model);
  EXPECT_EQ(cmdargs->status,RWCMDARGS_STATUS_SUCCESS);
  EXPECT_FALSE(cmdargs->editline);

  if (want_debug) {
    std::cout << std::endl << std::endl << "Dumping model:" << std::endl;
    rw_yang_model_dump(cmdargs->model, 2/*indent*/, 1/*verbosity*/);

    std::cout << std::endl << std::endl << "Dumping the current DOM:" << std::endl;
    cmdargs->manifest->to_stdout();
    std::cout << std::endl;
  }

  delete cmdargs;
  // ATTN: Assert that no memory leaked.
}


/**
 * The sole purpose of this test is to ensure that the
 * rwcmdargs_test_gen_init() auto-generated function actually works and
 * loads the correct yang file.
 */
TEST(RWCmdArgs, GeneratedInit) {
  TEST_DESCRIPTION("Generated code init function");
  bool want_debug = false;

  rwcmdargs_t* rwca = NULL;
  rw_status_t rs = rwcmdargs_test_gen_init(&rwca, want_debug);
  ASSERT_EQ(rs,RW_STATUS_SUCCESS);
  ASSERT_TRUE(rwca);
  EXPECT_EQ(rwcmdargs_status(rwca),RWCMDARGS_STATUS_SUCCESS);

  CmdArgs* cmdargs = static_cast<CmdArgs*>(rwca);
  ASSERT_TRUE(cmdargs);
  EXPECT_TRUE(cmdargs->parser);
  EXPECT_TRUE(cmdargs->xmlmgr);
  EXPECT_TRUE(cmdargs->manifest);
  EXPECT_TRUE(cmdargs->model);
  EXPECT_EQ(cmdargs->status,RWCMDARGS_STATUS_SUCCESS);
  EXPECT_FALSE(cmdargs->editline);

  if (want_debug) {
    std::cout << std::endl << std::endl << "Dumping model:" << std::endl;
    rw_yang_model_dump(cmdargs->model, 2/*indent*/, 1/*verbosity*/);

    std::cout << std::endl << std::endl << "Dumping the current DOM:" << std::endl;
    cmdargs->manifest->to_stdout();
    std::cout << std::endl;
  }

  rwcmdargs_free(rwca);
}


/**
 * Verify that the test XML file can be loaded and then dumped into a
 * protobuf.
 */
TEST(RWCmdArgs, ReadRawXML) {
  TEST_DESCRIPTION("Read raw XML directly - rwcmdargs-test.xml");
  bool want_debug = false;

  rwcmdargs_t* rwca = NULL;
  rw_status_t rs = rwcmdargs_test_gen_init(&rwca, want_debug);
  ASSERT_EQ(rs,RW_STATUS_SUCCESS);
  ASSERT_TRUE(rwca);
  EXPECT_EQ(rwcmdargs_status(rwca),RWCMDARGS_STATUS_SUCCESS);

  CmdArgs* cmdargs = static_cast<CmdArgs*>(rwca);
  ASSERT_TRUE(cmdargs);

  if (want_debug) {
    std::cout << std::endl << std::endl << "Dumping model:" << std::endl;
    rw_yang_model_dump(cmdargs->model, 2/*indent*/, 1/*verbosity*/);
  }

  ASSERT_GE(g_argc,2);
  ASSERT_TRUE(g_argv[1]);
  rs = cmdargs->load_from_xml(g_argv[1]);
  ASSERT_EQ(rs,RW_STATUS_SUCCESS);

  if (want_debug) {
    std::cout << std::endl << std::endl << "Dumping the current DOM:" << std::endl;
    cmdargs->manifest->to_stdout();
    std::cout << std::endl;
  }

  RWPB_M_MSG_DECL_INIT(RwcmdargsTest_RwcaTestCont,m);
  rs = cmdargs->save_to_pb(&m.base);
  ASSERT_EQ(rs,RW_STATUS_SUCCESS);
  ASSERT_NO_FATAL_FAILURE(verify_xml(m));

  delete cmdargs;
}


/**
 * This function tests the very basic workings of the CLI.
 */
TEST(RWCmdArgs, SimpleCli) {
  TEST_DESCRIPTION("CmdArgs test simple CLI command lines");
  bool want_debug = false;

  rwcmdargs_t* rwca = NULL;
  rw_status_t rs = rwcmdargs_test_gen_init(&rwca, want_debug);
  ASSERT_EQ(rs,RW_STATUS_SUCCESS);
  ASSERT_TRUE(rwca);
  EXPECT_EQ(rwcmdargs_status(rwca),RWCMDARGS_STATUS_SUCCESS);

  CmdArgs* cmdargs = static_cast<CmdArgs*>(rwca);
  ASSERT_TRUE(cmdargs);
  ASSERT_TRUE(cmdargs->parser);
  EXPECT_TRUE(cmdargs->xmlmgr);
  EXPECT_TRUE(cmdargs->manifest);
  EXPECT_TRUE(cmdargs->model);
  EXPECT_EQ(cmdargs->status,RWCMDARGS_STATUS_SUCCESS);
  EXPECT_FALSE(cmdargs->editline);

  if (want_debug) {
    std::cout << std::endl << std::endl << "Dumping model:" << std::endl;
    rw_yang_model_dump(cmdargs->model, 2/*indent*/, 1/*verbosity*/);

    std::cout << std::endl << std::endl << "Dumping the current DOM:" << std::endl;
    cmdargs->manifest->to_stdout();
    std::cout << std::endl;
  }

  std::string empty("");
  bool rv = cmdargs->parser->parse_line_buffer(empty,false);
  EXPECT_TRUE(rv);
  rv = cmdargs->parser->generate_help(empty);
  EXPECT_TRUE(rv);

  ASSERT_GE(g_argc,2);
  ASSERT_TRUE(g_argv[1]);
  std::string load("lo man f ");
  load += g_argv[1];
  rv = cmdargs->parser->parse_line_buffer(load,false);
  EXPECT_TRUE(rv);

  RWPB_M_MSG_DECL_INIT(RwcmdargsTest_RwcaTestCont,m);
  rs = cmdargs->save_to_pb(&m.base);
  ASSERT_EQ(rs,RW_STATUS_SUCCESS);
  ASSERT_NO_FATAL_FAILURE(verify_xml(m));

  delete cmdargs;
  // ATTN: Assert that no memory leaked.
}


/**
 * Tie it all together - verify that the auto-generated function can
 * parse a simple set of command line arguments that load the test XML
 * file.  Verify that it loaded successfully.
 */
TEST(RWCmdArgs, GeneratedParser) {
  TEST_DESCRIPTION("Generated code parser function");
  bool want_debug = false;

  RWPB_M_MSG_DECL_INIT(RwcmdargsTest_RwcaTestCont,m);

  char exe[]="exe";
  char load[]="load";
  char manifest[]="manifest";
  char file[]="file";
  char* argv[] = { exe, load, manifest, file, g_argv[1], NULL };
  int argc = sizeof(argv)/sizeof(argv[0]) - 1;

  rw_status_t rs = rwcmdargs_test_gen(&argc, argv, &m, want_debug);
  EXPECT_EQ(rs,RW_STATUS_SUCCESS);
  EXPECT_EQ(argc,1);

  ASSERT_NO_FATAL_FAILURE(verify_xml(m));
}


// rwcmdargs_test.cpp
