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

using namespace rw_yang;

rw_status_t test_rwutcli_show(int argc, const char** argv)
{
  printf("Called test_rwutcli_show():");
  for (int i = 0; i < argc; ++i) {
    printf(" %s", argv[i]);
  }
  printf("\n");
  return RW_STATUS_SUCCESS;
}

rw_status_t test_rwutcli_show_foo(int argc, const char** argv)
{
  printf("Called test_rwutcli_show_foo():");
  for (int i = 0; i < argc; ++i) {
    printf(" %s", argv[i]);
  }
  printf("\n");
  return RW_STATUS_SUCCESS;
}

rw_status_t test_rwutcli_show_bar(int argc, const char** argv)
{
  printf("Called test_rwutcli_show_bar():");
  for (int i = 0; i < argc; ++i) {
    printf(" %s", argv[i]);
  }
  printf("\n");
  return RW_STATUS_SUCCESS;
}


