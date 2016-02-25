
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */



/**
 * @file yangtest_common.hpp
 * @author Vinod Kamalaraj
 * @date 03/18/2015
 * @brief Test program for Generic tree iterators
 */

#ifndef __YANGTEST_COMMON_HPP_
#define __YANGTEST_COMMON_HPP_
#include "rw_ut_confd.hpp"
#include "rwut.h"
#include "yangncx.hpp"

#include <boost/scope_exit.hpp>

#include <confd_lib.h>
#include <confd_cdb.h>
#include <confd_dp.h>

#include <iostream>
#include "rw_tree_iter.h"
#include "rw_xml.h"
#include "confd_xml.h"
#include "flat-conversion.confd.h"
#include "bumpy-conversion.pb-c.h"
#include "flat-conversion.pb-c.h"
#include "other-data_rwvcs.pb-c.h"
#include "rift-cli-test.pb-c.h"
#include "test-ydom-top.pb-c.h"

extern char **g_argv;
extern int g_argc;

RWPB_T_MSG(RiftCliTest_data_GeneralContainer) *get_general_containers(int num_list_entries);

typedef UniquePtrProtobufCMessage<>::uptr_t msg_uptr_t;

rw_status_t rw_test_build_confd_array(const char *harness_name,
                                      confd_tag_value_t **array,
                                      size_t            *count,
                                      struct confd_cs_node **ret_cs_node);

rw_status_t rw_test_build_pb_flat_from_file (const char *filename,
                                             ProtobufCMessage **msg);

rw_status_t rw_test_build_pb_bumpy_from_file (const char *filename,
                                              ProtobufCMessage **msg);


std::string get_rift_root();

#endif
