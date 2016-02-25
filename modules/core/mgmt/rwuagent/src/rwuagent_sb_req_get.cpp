
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwuagent_sb_req_get.cpp
 *
 * Management agent southbound request support for reads
 */

#include "rwuagent.hpp"

using namespace rw_uagent;
using namespace rw_yang;


SbReqGet::SbReqGet(
    Instance* instance,
    NbReq* nbreq,
    const char *xml_fragment)
: SbReq(
    instance,
    nbreq,
    RW_MGMTAGT_SB_REQ_TYPE_GETNETCONF,
    "SbReqGetString",
    xml_fragment )
{
  // Let any execptions propagate upwards - the clients that start a transaction
  // should handle it?
  RW_MA_SBREQ_LOG (this, __FUNCTION__, xml_fragment);

  // Create a response DOM that can be used to build up the responses from
  // members. The DOMs lowest level is the GET node

  std::string error_out;
  rsp_dom_ =
      std::move(instance_->xml_mgr()->create_document_from_string(xml_fragment, error_out, false));

  XMLNode *node = rsp_dom_->get_root_node()->get_first_element();

  ProtobufCMessage *msg = nullptr;
  rw_yang_netconf_op_status_t ncs = node->to_pbcm (&msg);
  if (RW_YANG_NETCONF_OP_STATUS_OK == ncs) {
    message_ =  pbcm_uptr_t (msg);
  }
  update_stats(RW_UAGENT_NETCONF_PARSE_REQUEST);
}

SbReqGet::SbReqGet(
    Instance* instance,
    NbReq* nbreq,
    XMLDocument::uptr_t cmd)
: SbReq(
    instance,
    nbreq,
    RW_MGMTAGT_SB_REQ_TYPE_GETNETCONF,
    "SbReqGetDom" )
{
  rsp_dom_ = std::move(cmd);
  XMLNode *node = rsp_dom_->get_root_node()->get_first_element();
  std::string capture_temporary;
  RW_MA_SBREQ_LOG (this, __FUNCTION__, (capture_temporary=node->to_string()).c_str());

  ProtobufCMessage *msg = nullptr;
  rw_yang_netconf_op_status_t ncs = node->to_pbcm (&msg);
  message_ = pbcm_uptr_t (msg);
  RW_ASSERT (RW_YANG_NETCONF_OP_STATUS_OK == ncs);
  update_stats (RW_UAGENT_NETCONF_PARSE_REQUEST);
}

SbReqGet::~SbReqGet ()
{
  if (xact_) {
    rwdts_xact_unref (xact_, __PRETTY_FUNCTION__, __LINE__);
    xact_ = nullptr;
  }
  instance_->update_stats(sbreq_type(), req_.c_str(), &statistics_);
}

void SbReqGet::async_dispatch_f(void* cb_data)
{
  SbReqGet *get = static_cast<SbReqGet *>(cb_data);
  RW_ASSERT (get);

  switch (get->get_async_dt()) {
    
    case async_dispatch_t::DTS_QUERY:
      get->start_dts_xact_int();
      break;

    case async_dispatch_t::DTS_SSD:
      get->start_xact_show_support_details();
      break;

    case async_dispatch_t::DTS_CCB:
      get->commit_cb (get->dts_xact());
      break;

    default:
      RW_ASSERT_NOT_REACHED();
  }
}

void SbReqGet::start_dts_xact_int()
{
  RW_ASSERT(query_ks_.get());

  xact_ = rwdts_api_query_ks(dts_->api(),
                             query_ks_.get(),
                             RWDTS_QUERY_READ,
                             RWDTS_XACT_FLAG_NOTRAN|RWDTS_FLAG_WAIT_RESPONSE|dts_flags_,
                             dts_get_event_cb,
                             this,
                             0);

  RW_ASSERT(xact_);
  rwdts_xact_ref (xact_, __PRETTY_FUNCTION__, __LINE__); // Take a reference always
  update_stats(RW_UAGENT_NETCONF_DTS_XACT_START);
}

StartStatus SbReqGet::start_xact_int()
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "start get");

  // Check if this is a get request for UAGENT. If it is, respond.
  XMLNode *node = rsp_dom_->get_root_node()->get_first_element();

  if (!message_.get()) {
    RW_MA_SBREQ_LOG (this, __FUNCTION__, "Failed");
    /* ATTN: Better error handling */
    return done_with_error("no data path");
  }

  // check if the first child of root is uagent - if so, handle locally.
  node = rsp_dom_->get_root_node()->get_first_element();
  if (node->get_name_space() == uagent_yang_ns) {
    ::async_execute(
        instance_->rwsched_tasklet(),
        rwsched_dispatch_get_main_queue(instance_->rwsched()),
        this,
        [](void* ctxt)-> void {
          auto* self = static_cast<SbReqGet*>(ctxt);
          self->start_xact_uagent_get_request();
        }
    );
    return StartStatus::Async;
  }

  // Find the deepest node, and convert to keyspec
  node = rsp_dom_->get_root_node()->find_first_deepest_node();
  if (node->get_yang_node() && 
          node->get_yang_node()->is_leafy()) {
    node = node->get_parent();
  }
  rw_keyspec_path_t* key = nullptr;
  rw_yang_netconf_op_status_t ncrs = node->to_keyspec(&key);

  if (ncrs != RW_YANG_NETCONF_OP_STATUS_OK) {
    return done_with_error("to_keyspec failure");
  }

  rw_keyspec_path_set_category (key, NULL , RW_SCHEMA_CATEGORY_DATA);

  char ks_print_buffer[RWUAGENT_KS_DEBUG_BUFFER_LENGTH];
  rw_keyspec_path_get_print_buffer(key, NULL , NULL, ks_print_buffer, sizeof(ks_print_buffer));
  req_ = ks_print_buffer;
  RW_MA_SBREQ_LOG (this, __FUNCTION__,ks_print_buffer);

  query_ks_.reset(key);

  ad_type_ = async_dispatch_t::DTS_QUERY;
  rwsched_dispatch_async_f(instance_->rwsched_tasklet(),
                           rwsched_dispatch_get_main_queue(instance_->rwsched()),
                           this,
                           async_dispatch_f);

  return StartStatus::Async;
}

StartStatus SbReqGet::start_xact_uagent_get_request()
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "get local agent data" );

  XMLNode *node = rsp_dom_->get_root_node()->get_first_element();
  RW_ASSERT(node);

  if ("system" == node->get_local_name()) {

    ad_type_ = async_dispatch_t::DTS_SSD;
    rwsched_dispatch_async_f(instance_->rwsched_tasklet(),
                             rwsched_dispatch_get_main_queue(instance_->rwsched()),
                             this,
                             async_dispatch_f);

    return StartStatus::Async;
  }

  // The first node is state
  node = node->get_first_child();

  if (nullptr == node) {
    return done_with_error( "Could not parse get request" );
  }

  /* ATTN: This whole code has to change at some time in the near future.
   *  Currently, this data can be retrieved only through MgmtAgent and not from DTS
   * Also, this is badly written - should most probably use maps or such.
   */
  typedef rw_yang_netconf_op_status_t (SbReqGet::* child_handler)(rw_yang::XMLNode *);

  const char **names = NULL;
  child_handler *handlers = NULL;
  uint8_t num_handlers;

  const char *state_node_names[] = {"statistics", "specific"};
  child_handler state_handlers[] = {&SbReqGet::get_statistics, &SbReqGet::get_specific};

  const char *confd_node_names[] = {"client", "dom-refresh-period"};
  child_handler confd_handlers[] = {&SbReqGet::get_confd_daemon, &SbReqGet::get_dom_refresh_period};

  if ("state" == node->get_local_name()) {
    names = state_node_names;
    num_handlers = sizeof (state_node_names)/ sizeof (char *);
    handlers = state_handlers;
  } else if ("confd" == node->get_local_name()) {
    names = confd_node_names;
    num_handlers = sizeof (confd_node_names)/sizeof (char *);
    handlers = confd_handlers;
  } else if ("last-error" == node->get_local_name()) {
    // ATTN need to implement this
  } else {
    return done_with_error( "No known management agent state" );
  }

  // This would need to get refactored
  if (nullptr == node->get_first_child()) {
    // Add each branch, and get that data

    for (int i = 0; i < num_handlers; i++) {
      (this->*handlers[i])(node);
    }

  } else {
    XMLNode *child = node->get_first_child();
    RW_ASSERT(child);
    for (size_t i = 0; i < num_handlers; i++) {
      if (names[i] == child->get_local_name()) {
        node->remove_child (child);
        (this->*handlers[i])(node);
        break;
      }
    }
    // Node is an empty list - remove it
  }

  // ATTN:- I was not sure whether the earlier async was to avoid blocking
  // in a thread for long time or to be consistent with the dts gets. 
  // I have made this to return synchronously as we are on
  // a separate queue now. If i find any issues, i will make it async.
  done_with_success( std::move(rsp_dom_) );
  return StartStatus::Done;
}

StartStatus SbReqGet::start_xact_show_support_details()
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "support details" );

  xact_ = rwdts_api_xact_create(
    dts_->api(),
    RWDTS_XACT_FLAG_NOTRAN|RWDTS_FLAG_WAIT_RESPONSE|dts_flags_,
    dts_get_event_cb,
    this);

  RW_ASSERT (xact_);
  rwdts_xact_ref (xact_, __PRETTY_FUNCTION__, __LINE__); // always take a ref

  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "xact dts created",
    RWMEMLOG_ARG_PRINTF_INTPTR("dts xact=0x%" PRIXPTR, (intptr_t)xact_) );

  /**********************************************************************
   ************ CAUTION ** CAUTION  ** CAUTION  ** CAUTION  ** CAUTION **
   ** Inform Automation team when this list is modified, so that tests **
   ** are updated *******************************************************
   ************ CAUTION ** CAUTION  ** CAUTION  ** CAUTION  ** CAUTION **
   **********************************************************************/
  const char *xpaths[] = {
    "A,/rw-base:vcs/rw-base:info",
    "A,/rw-base:vcs/rw-base:resources",
    "D,/rwmsg:messaging/rwmsg:info",
    "D,/rwdts:dts/rwdts:member",
    "D,/rwdts:dts/rwdts:routers"
  };
  /**********************************************************************
   ************ CAUTION ** CAUTION  ** CAUTION  ** CAUTION  ** CAUTION **
   ** Inform Automation team when this list is modified, so that tests **
   ** are updated *******************************************************
   ************ CAUTION ** CAUTION  ** CAUTION  ** CAUTION  ** CAUTION **
   **********************************************************************/

  for (size_t i = 0; i < sizeof(xpaths)/sizeof(xpaths[0]);  i++) {
    rw_keyspec_path_t *key;
    auto rs = rwdts_api_keyspec_from_xpath(dts_->api(), xpaths[i], &key);
    if (rs != RW_STATUS_SUCCESS) {
      return done_with_error("Failed in xpath to ketspec");
    }

    rwdts_xact_block_t *blk = rwdts_xact_block_create(xact_);
    RW_ASSERT(blk);

    rs = rwdts_xact_block_add_query_ks(
      blk,
      key,
      RWDTS_QUERY_READ,
      RWDTS_XACT_FLAG_NOTRAN|RWDTS_FLAG_WAIT_RESPONSE|RWDTS_FLAG_BLOCK_MERGE,
      0,
      nullptr);
    if (rs != RW_STATUS_SUCCESS) {
      return done_with_error("Failed in add dts query block");
    }

    rs = rwdts_xact_block_execute(blk, dts_flags_, NULL, NULL, NULL);
    RW_ASSERT(rs == RW_STATUS_SUCCESS);
  }

  rw_status_t rs = rwdts_xact_commit(xact_);
  RW_ASSERT(RW_STATUS_SUCCESS == rs);

  update_stats(RW_UAGENT_NETCONF_DTS_XACT_START);
  return StartStatus::Async;
}

rw_yang_netconf_op_status_t SbReqGet::get_dom_refresh_period(
  XMLNode *node)
{
  RWMEMLOG_TIME_SCOPE(memlog_buf_, RWMEMLOG_MEM2, "get agent refresh" );
  RW_ASSERT(node);

  RWPB_M_MSG_DECL_INIT(RwMgmtagt_Confd, confd);
  RWPB_M_MSG_DECL_INIT(RwMgmtagt_Confd_DomRefreshPeriod, refresh_period);

  // ATTN: confd.has_total_dom_lifetime = true;
  confd.dom_refresh_period = &refresh_period;

  refresh_period.has_cli_dom_refresh_period = true;
  refresh_period.cli_dom_refresh_period = instance_->cli_dom_refresh_period_msec_;

  refresh_period.has_nc_rest_dom_refresh_period = true;
  refresh_period.nc_rest_dom_refresh_period = instance_->nc_rest_refresh_period_msec_;

  rw_yang_netconf_op_status_t ncs = node->merge( &confd.base );
  return ncs;
}

rw_yang_netconf_op_status_t SbReqGet::get_confd_daemon(
  XMLNode *node)
{
  RWMEMLOG_TIME_SCOPE(memlog_buf_, RWMEMLOG_MEM2, "get agent confd stats" );
  RW_ASSERT(node);

  RWPB_M_MSG_DECL_INIT(RwMgmtagt_Confd, confd);
  RWPB_T_MSG(RwMgmtagt_Client) *client = (RWPB_T_MSG(RwMgmtagt_Client) *)
      protobuf_c_message_create(nullptr, RWPB_G_MSG_PBCMD(RwMgmtagt_Client));

  confd.n_client = 1;
  confd.client = &client;

  client->n_cached_dom = instance_->confd_daemon_->dom_stats_.size();
  RWPB_T_MSG(RwMgmtagt_CachedDom) **doms = (RWPB_T_MSG(RwMgmtagt_CachedDom) **)
      //ATTN this has to be based on some alloc on the protobuf allocator
      RW_MALLOC(sizeof(RWPB_T_MSG(RwMgmtagt_CachedDom) *)  * client->n_cached_dom);

  client->cached_dom = doms;
  client->identifier = 0;

  int i = 0; // fake index
  for (auto& dom : instance_->confd_daemon_->dom_stats_) {
    doms[i] = dom->get_pbcm();
    doms[i]->index = i;
    i++;
  }

  rw_yang_netconf_op_status_t ncrs =
      node->merge ((ProtobufCMessage *)&confd);

  if (ncrs != RW_YANG_NETCONF_OP_STATUS_OPERATION_FAILED) {
    RW_MA_SBREQ_LOG (this, __FUNCTION__, "Failed in merge op");
    return RW_YANG_NETCONF_OP_STATUS_OPERATION_FAILED;
  }

  RW_FREE ( doms);
  client->n_cached_dom = 0;
  client->cached_dom = nullptr;

  for (auto dp : instance_->confd_daemon_->dp_clients_) {
    client->identifier = (uint64_t) dp;
    client->n_cached_dom = dp->dom_stats_.size();
    doms = (RWPB_T_MSG(RwMgmtagt_CachedDom) **)
        //ATTN this has to be based on some alloc on the protobuf allocator
        RW_MALLOC (sizeof(RWPB_T_MSG(RwMgmtagt_CachedDom)*)  * client->n_cached_dom);

    client->cached_dom = doms;
    int i = 0; // fake index

    for (auto& dom : dp->dom_stats_) {
      doms[i] = dom->get_pbcm();
      doms[i]->index = i;
      i++;
    }

    rw_yang_netconf_op_status_t ncrs =
        node->merge ((ProtobufCMessage *)&confd);

    if (ncrs != RW_YANG_NETCONF_OP_STATUS_OK) {
      RW_MA_SBREQ_LOG (this, __FUNCTION__, "Failed in merge op");
      return RW_YANG_NETCONF_OP_STATUS_OPERATION_FAILED;
    }

    RW_FREE ( doms);
    client->n_cached_dom = 0;
    client->cached_dom = nullptr;
  }

  protobuf_c_message_free_unpacked (nullptr, (ProtobufCMessage*)client);
  return RW_YANG_NETCONF_OP_STATUS_OK;
}

rw_yang_netconf_op_status_t SbReqGet::get_statistics(
  XMLNode *node)
{
  RWMEMLOG_TIME_SCOPE(memlog_buf_, RWMEMLOG_MEM2, "get agent stats" );
  RW_ASSERT(node);

  //ATTN: filter based on operation - Add  a function in rw_xml that returns an
  // enum value for a enum field.

  // ATTN: Temporary work around for RW_XML not merging correctly at
  // a listy node

  RWPB_M_MSG_DECL_INIT(RwMgmtagt_State, state);

  // Send all the statistics (for now)
  for (size_t i = 0; i < RW_MGMTAGT_SB_REQ_TYPE_MAXIMUM; i++) {
    if (instance_->statistics_[i]) {

      RWPB_T_MSG(RwMgmtagt_Statistics) *statistics = &instance_->statistics_[i]->statistics;

      statistics->has_elapsed_time = 1;
      struct timeval now;
      gettimeofday (&now, 0);
      RW_ASSERT(timercmp(&now, &instance_->statistics_[i]->start_time_, >=));
      struct timeval elapsed;
      timersub (&now, &instance_->statistics_[i]->start_time_, &elapsed);
      statistics->elapsed_time = elapsed.tv_sec;

      state.n_statistics = 1;
      state.statistics = &statistics;

      rw_yang_netconf_op_status_t ncrs =
          node->merge ((ProtobufCMessage *)&state);

      if (ncrs != RW_YANG_NETCONF_OP_STATUS_OK) {
        RW_MA_SBREQ_LOG (this, __FUNCTION__, "Failed in merge op");
        return RW_YANG_NETCONF_OP_STATUS_OPERATION_FAILED;
      }

    }
  }
  return RW_YANG_NETCONF_OP_STATUS_OK;
}

rw_yang_netconf_op_status_t SbReqGet::get_specific(
  XMLNode *node)
{
  RWMEMLOG_TIME_SCOPE(memlog_buf_, RWMEMLOG_MEM2, "get specifc stats" );
  //ATTN: filter based on operation - Add  a function in rw_xml that returns an
  // enum value for a enum field.

  // ATTN: Temporary work around for RW_XML not merging correctly at
  // a listy node

  RWPB_M_MSG_DECL_INIT(RwMgmtagt_State, state);

  // Send all the specifics for now
  for (size_t i = 0; i < RW_MGMTAGT_SB_REQ_TYPE_MAXIMUM; i++) {

    RWPB_M_MSG_DECL_INIT(RwMgmtagt_Specific, specific);
    RWPB_T_MSG (RwMgmtagt_Specific) *spec_ptr = &specific;

    state.n_specific = 1;
    state.specific = &spec_ptr;

    specific.operation = (RwMgmtagt_SbReqType)i;

    if (instance_->statistics_[i]) {
      RWPB_T_MSG(RwMgmtagt_SpecificStatistics) commands[RWUAGENT_MAX_CMD_STATS];
      RWPB_T_MSG(RwMgmtagt_SpecificStatistics) *cmd_ptr[RWUAGENT_MAX_CMD_STATS];
      RWPB_T_MSG(RwMgmtagt_SpecificStatistics_ProcessingTimes)
          processing[RWUAGENT_MAX_CMD_STATS];
      specific.commands = cmd_ptr;
      size_t j = 0;

      for (auto specific_entry :instance_->statistics_[i]->commands) {
        RWPB_F_MSG_INIT (RwMgmtagt_SpecificStatistics, &commands[j]);
        RWPB_F_MSG_INIT (RwMgmtagt_SpecificStatistics_ProcessingTimes, &processing[j]);
        commands[j].index = j;
        commands[j].request = const_cast <char *> (specific_entry.request.c_str());
        processing[j] = specific_entry.statistics;
        commands[j].processing_times = &processing[j];
        cmd_ptr[j] = &commands[j];
        j++;
      }

      specific.n_commands = j;

      // ATTN: Temporary work around for RW_XML not merging correctly at
      // a listy node

      rw_yang_netconf_op_status_t ncrs =
          node->merge ((ProtobufCMessage *)&state);

      if (ncrs != RW_YANG_NETCONF_OP_STATUS_OK) {
        RW_MA_SBREQ_LOG (this, __FUNCTION__, "Failed in merge op");
        return RW_YANG_NETCONF_OP_STATUS_OPERATION_FAILED;
      }
    }
  }

  return RW_YANG_NETCONF_OP_STATUS_OK;
}

void SbReqGet::dts_get_event_cb(rwdts_xact_t *xact,
                                rwdts_xact_status_t* xact_status,
                                void *ud)
{
  if (xact_status->xact_done) {
    SbReqGet *get =  static_cast<SbReqGet *> (ud);
    RW_ASSERT (get);

    RW_ASSERT (xact == get->dts_xact());

    get->set_async_dt(async_dispatch_t::DTS_CCB);
    rwsched_dispatch_async_f(get->instance()->rwsched_tasklet(),
                             get->instance()->cc_dispatchq(),
                             get,
                             async_dispatch_f);
  }
}

void SbReqGet::commit_cb( rwdts_xact_t *xact )
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "dts query complete");
  RW_MA_SBREQ_LOG (this, __FUNCTION__, "");
  done_with_success( xact );
}

