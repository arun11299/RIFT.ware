
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/*!
 * @file rwuagent.hpp
 * @brief Private micro-agent header file
 */

#ifndef CORE_MGMT_RWUAGENT_HPP_
#define CORE_MGMT_RWUAGENT_HPP_

#include <array>
#include <chrono>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <boost/optional.hpp>

#include <rw_xml.h>
#include <rwmemlog.h>
#include <rwmemlog_mgmt.h>
#include <rwmemlogdts.h>
#include <rwdts.h>
#include <rwmsg.h>

#include <rwuagent.h>
#include "rw-mgmtagt.pb-c.h"
#include <rw-mgmtagt-log.pb-c.h>
#include "rw-mgmt-schema.pb-c.h"
#include "rwuagent_startup.hpp"

#include "rwuagent_dynamic_schema.hpp"
#include "yangncx.hpp"
#include "confd_xml.h"

namespace rw_uagent {

extern const char *uagent_yang_ns;

const int RWUAGENT_MAX_CMD_STATS = 10;
const int RWUAGENT_KS_DEBUG_BUFFER_LENGTH = 1024;
const int RWUAGENT_INVALID_SOCK_ID = -1;
const int RWUAGENT_NOTIFICATION_TIMEOUT = 10;

// DOM cleanup timer related constants
static const int RWUAGENT_DOM_CLI_TIMER_PERIOD_MSEC = 120000;
static const int RWUAGENT_DOM_NC_REST_TIMER_PERIOD_MSEC = 250;

constexpr const uint32_t RWUAGENT_SCHEMA_MAX_VAL = std::numeric_limits<uint32_t>::max();


class MsgClient;
class NbReq;
class NbReqDts;
class SbReq;
class SbReqEditConfig;
class SbReqGet;
class SbReqRpc;
class DtsMember;
class Instance;


/// NbReq token value. No local meaning.
typedef intptr_t token_t;
typedef std::chrono::high_resolution_clock clock_t;

enum class StartStatus {
  Async = 0x5748,
  Done,
};

typedef enum dom_stats_state_t {
  RW_UAGENT_NETCONF_REQ_RVCVD,
  RW_UAGENT_NETCONF_PARSE_REQUEST,
  RW_UAGENT_NETCONF_DTS_XACT_START,
  RW_UAGENT_NETCONF_DTS_XACT_DONE,
  RW_UAGENT_NETCONF_RESPONSE_BUILT
} dom_stats_state_t;

typedef RWPB_E(RwNetconf_ErrorType) RwNetconf_ErrorType;
typedef RWPB_E(IetfNetconf_ErrorTagType) IetfNetconf_ErrorTagType;
typedef RWPB_E(IetfNetconf_ErrorSeverityType) IetfNetconf_ErrorSeverityType;
typedef RWPB_E(RwMgmtagt_SbReqType) RwMgmtagt_SbReqType;
typedef RWPB_E(RwMgmtagt_NbReqType) RwMgmtagt_NbReqType;
typedef RWPB_E(RwMgmtSchema_ApplicationState) RwMgmtSchema_ApplicationState;

typedef RWPB_T_MSG(RwMgmtagt_data_Uagent_LastError_RpcError) IetfNetconfErrorReply;
typedef RWPB_T_MSG(RwMgmtagt_input_MgmtAgent_PbRequest) RwMgmtagt_PbRequest;
typedef RWPB_T_MSG(RwMgmtagt_output_MgmtAgent_PbRequest) RwMgmtagt_PbResponse;

typedef UniquePtrProtobufCMessage<>::uptr_t pbcm_uptr_t;
typedef UniquePtrKeySpecPath::uptr_t ks_uptr_t;


/*
 * Logging Macros for UAGENT
 */
#define RW_MA_Paste3(a,b,c) a##_##b##_##c

#define RW_MA_INST_LOG( instance_, evvtt, ... ) \
  RW_MA_INST_LOG_Step1( \
    instance_, \
    RW_MA_Paste3(RwMgmtagtLog, \
    notif, \
    evvtt), \
    __VA_ARGS__ )

#define RW_MA_INST_LOG_Step1(instance_, evvtt, ... ) \
  RWLOG_EVENT( \
    (instance_)->rwlog(), \
    evvtt, \
    (uint64_t)instance_, \
    __VA_ARGS__)

#define RW_MA_NBREQ_LOG( nbreq_, evvtt, ... ) \
  RW_MA_INST_NBREQ_LOG_Step1( \
    nbreq_, \
    RW_MA_Paste3(RwMgmtagtLog, \
    notif, \
    evvtt), \
    __VA_ARGS__ )

#define RW_MA_INST_NBREQ_LOG_Step1( nbreq_, evvtt, ... ) \
  RWLOG_EVENT( \
    nbreq_->instance()->rwlog(), \
    evvtt, \
    nbreq_->nbreq_type(), \
    (uint64_t) nbreq_, \
    __VA_ARGS__ )

#define RW_MA_NBREQ_LOG_FD( nbreq_, ... ) \
  RWLOG_EVENT( \
    nbreq_->instance()->rwlog(), \
    RwMgmtagtLog_notif_ClientFdLog, \
    nbreq_->nbreq_type(), \
    (uint64_t) nbreq_, \
    __VA_ARGS__ )

#define RW_MA_INST_DETAIL( instance_, ... ) \
  RWLOG_EVENT( \
    (instance_)->rwlog(), \
    RwMgmtagtLog_notif_InstanceDebugDetailed, \
    (uint64_t)instance_, \
    __VA_ARGS__ )

#define RW_MA_SBREQ_LOG( sbreq_, ... ) \
  RWLOG_EVENT( \
    (sbreq_->nbreq_->instance())->rwlog(), \
    RwMgmtagtLog_notif_ClientSbReqDebug, \
    sbreq_->sbreq_type(), \
    (uint64_t)sbreq_, \
    sbreq_->nbreq_->nbreq_type(), \
    (uint64_t) sbreq_->nbreq_, \
    __VA_ARGS__ )

#define RW_MA_DOMMGR_LOG( instance_, evvtt, ... ) \
  RW_MA_DOMMGR_LOG_Step1( \
    (instance_), \
    RW_MA_Paste3(RwMgmtagtLog, \
    notif, \
    evvtt), \
    __VA_ARGS__ )

#define RW_MA_DOMMGR_LOG_Step1( instance_, evvtt, ... ) \
  RWLOG_EVENT( \
    (instance_)->rwlog(), \
    evvtt, \
    __VA_ARGS__ )


/*
 * A convenience function for dispatching tasks (code block)
 * on dispatch thread/queue. 
 * Callers are responsible for providing correct signature
 * for the std function via lambdas (preferrable way)
 */
static inline 
void async_execute(rwsched_tasklet_ptr_t tasklet,
                   rwsched_dispatch_queue_t launch_q,
                   void* ud,
                   dispatch_function_t task)
{
  //typedef void (*dispatch_func)(void*);

  rwsched_dispatch_async_f(
                          tasklet,
                          launch_q,
                          ud,
                          task);

  return;
}


/*!
 * Encapsulation for 'rcp-error' element specified in the Netconf RFC.
 * There can be mutliple NetconfError's sent in a single error response.
 */
class NetconfError
{
public:
  NetconfError(
    RwNetconf_ErrorType errType = RW_NETCONF_ERROR_TYPE_APPLICATION,
    IetfNetconf_ErrorTagType errTag = IETF_NETCONF_ERROR_TAG_TYPE_OPERATION_FAILED
  );
  ~NetconfError();

  /*!
   * Defines the conceptual layer where the error occurred.
   * Possible values are transport, rpc, protocol, application
   */
  NetconfError& set_type(RwNetconf_ErrorType type);

  /*!
   * Contains the enum identifying the error condition. Possible values are
   * provided in Netconf RFC 6241 Appendix A.
   */
  NetconfError& set_tag(IetfNetconf_ErrorTagType tag);

  /*!
   * Get the IETF Netconf error tag
   */
  IetfNetconf_ErrorTagType get_tag() const;

  /*!
   * Converts the RW netconf status (which is also based on Netconf error_tag)
   * to Netconf error_tag value.
   */
  NetconfError& set_rw_error_tag(rw_yang_netconf_op_status_t status);

  /*!
   * Converts the RW status to Netconf error_tag value.
   */
  NetconfError& set_rw_error_tag(rw_status_t rwstatus);

  /*!
   * Converts the error tag value to RW netconf status.
   */
  rw_yang_netconf_op_status_t get_rw_nc_error_tag() const;

  /*!
   * Identifies the error severity. Possible values are error and warning.
   * Not currently used. Reserved for future use.
   */
  NetconfError& set_severity(IetfNetconf_ErrorSeverityType severity);

  /*!
   * Contains a string identifying the data-model-specific or
   * implementation-specific error condition, if one exists.
   */
  NetconfError& set_app_tag(const char* app_tag);

  /*!
   * Get the application tag
   */
  const char* get_app_tag() const;

  /*!
   * Contains the absolute XPath expression identifying the element path to
   * the node that is associated with the error.
   */
  NetconfError& set_error_path(const char* err_path);

  /*!
   * Get the xpath for the error
   */
  const char* get_error_path() const;

  /*!
   * Contains a string suitable for human display that describes the error
   * condition.
   */
  NetconfError& set_error_message(const char* err_msg);


  NetconfError& set_errno(const char* sysc);

  /*!
   * Get the message for the error
   */
  const char* get_error_message() const;

  /*!
   * Contains protocol- or data-model-specific error content in XML format.
   */
  NetconfError& set_error_info(const char* err_info);

  // Used internally by the NetconfErrorList while forming XML
  IetfNetconfErrorReply* get_pb_error();

private:

  /*!
   * Yang model defined protobuf object for 'rpc-error' element
   */
  IetfNetconfErrorReply error_;
};

/*!
 * Encapsulation for the list of 'rpc-error' elements
 *
 * Sample usage:
 * <code>
 *   NetconfErrorList nc_err_list(2); // two errors to be reported
 *   nc_err_list.add_error()
 *     .set_type(RW_YANG_TYPES_ERROR_TYPE_PROTOCOL)
 *     .set_tag(RW_YANG_TYPES_ERROR_TAG_MISSING_ATTRIBUTE)
 *     .set_error_message("Attribute Message-Id missing");
 *
 *   nc_err_list.add_error()
 *     .set_type(RW_YANG_TYPES_ERROR_TYPE_APPLICATION)
 *     .set_tag(RW_YANG_TYPES_ERROR_TAG_ACCESS_DENIED)
 *     .set_error_message("Access to the specified resource denied");
 * </code>
 */
class NetconfErrorList
{
public:
  /*!
   * Initializes a NetconfError list.
   */
  NetconfErrorList();

  /*!
   * Add an error to the error list.
   */
  NetconfError& add_error(
    RwNetconf_ErrorType errType = RW_NETCONF_ERROR_TYPE_APPLICATION,
    IetfNetconf_ErrorTagType errTag = IETF_NETCONF_ERROR_TAG_TYPE_OPERATION_FAILED
  );

  /*!
   * Return the number of errors in the list.
   */
  size_t length() const
  {
    return errors_.size();
  }

  /*!
   * Access error entries
   */
  const std::vector<NetconfError>& get_errors() const;

  /*!
   * Converts the rpc-error protocol buffer to XML format.
   */
  bool to_xml(rw_yang::XMLManager* xml_mgr, std::string& xml);

private:

  /*!
   * An array of NetconfError's that will be sent with the error response.
   */
  std::vector<NetconfError> errors_;
};

// The command and its stats in a tuple
typedef struct CommandStat {
  std::string  request;
  RWPB_T_MSG(RwMgmtagt_SpecificStatistics_ProcessingTimes) statistics;
} CommandStat;

typedef std::list<CommandStat> CommandStats;

typedef struct OperationalStats { //  OperationalStats
  struct timeval                 start_time_;
  RWPB_T_MSG(RwMgmtagt_Statistics)     statistics;
  CommandStats                        commands;
} OperationalStats;

}


// ATTN: This is ugly, unwind the dependencies some more!
#include "rwuagent_nb_req.hpp"
#include "rwuagent_sb_req.hpp"
#include "rwuagent_confd.hpp"


namespace rw_uagent {


/*!
 * Dts instance.
 */
class DtsMember
{
 public:
  /*!
   * Construct the DTS member API.
   */
  DtsMember(
    Instance* instance );

  /*!
   * Destroy the DTS member API.
   */
  ~DtsMember();

 private:
  // cannot copy
  DtsMember(const DtsMember&) = delete;
  DtsMember& operator=(const DtsMember&) = delete;

 public:
  /*!
   * Get the memlog buffer for DtsMember.
   */
  rwmemlog_buffer_t** get_memlog_ptr()
  {
    return &memlog_buf_;
  }

  /*!
   * Return the DTS handle.
   */
  rwdts_api_t* api() const
  {
    return api_;
  }

  /*!
   * Get flags for a transaction.
   */
  RWDtsFlag get_flags();

  /*!
   * Check if DTS is ready for queries.
   */
  bool is_ready() const;

  /*!
   * Trace next DTS transaction.
   */
  void trace_next();

  /*!
   * Register the RPC callback.
   */
  void register_rpc();

  /*!
   * Registers the nodes defined for the provided model
   */
  void load_registrations(rw_yang::YangModel* model);

private:

  static void state_change_cb(
    rwdts_api_t* apih,
    rwdts_state_t state,
    void* ud );

  void state_change(
    rwdts_state_t state );

  void dts_state_init();

  void dts_state_config();

  static rwdts_member_rsp_code_t notification_recv_cb(
      const rwdts_xact_info_t * xact_info,
      RWDtsQueryAction action,
      const rw_keyspec_path_t * keyspec,
      const ProtobufCMessage * msg,
      uint32_t credits,
      void * get_next_key);

  static rwdts_member_rsp_code_t get_config_cb(
    const rwdts_xact_info_t* xact_info,
    RWDtsQueryAction action,
    const rw_keyspec_path_t* ks,
    const ProtobufCMessage* msg,
    uint32_t credits,
    void* getnext_ptr );

  rwdts_member_rsp_code_t get_config(
    rwdts_xact_t* xact,
    rwdts_query_handle_t qhdl,
    const rw_keyspec_path_t* ks );

  static rwdts_member_rsp_code_t rpc_cb(
    const rwdts_xact_info_t* xact_info,
    RWDtsQueryAction action,
    const rw_keyspec_path_t* ks_in,
    const ProtobufCMessage* msg,
    uint32_t credits,
    void* getnext_ptr );

  rwdts_member_rsp_code_t rpc(
    const rwdts_xact_info_t* xact_info,
    RWDtsQueryAction action,
    const rw_keyspec_path_t* ks_in,
    const ProtobufCMessage* msg );


private:

  //! The owning uAgent.
  Instance* instance_ = nullptr;

  //! rwmemlog logging buffer
  RwMemlogBuffer memlog_buf_;

  //! DTS API handle
  rwdts_api_t* api_ = nullptr;

  //! Has registration with DTS completed?
  bool ready_ = false;

  //! A flag to be set on the next DTS transaction
  RWDtsFlag flags_ = RWDTS_FLAG_NONE;

  //! RPC registration handle.
  rwdts_member_reg_handle_t rpc_reg_;

  //! List of DTS registrations (config and notification)
  std::list<rwdts_member_reg_handle_t> all_dts_regs_;
};


/*!
 * An internal messaging based client.  All messaging-based clients
 * funnel through a single instance of this class.
 */
class MsgClient
{
public:
  MsgClient(Instance* instance);
  ~MsgClient();

  // cannot copy
  MsgClient(const MsgClient&) = delete;
  MsgClient& operator=(const MsgClient&) = delete;

public:

  /*!
   * rwmsg callback for RwUAgent.netconf_request
   */
  static void reqcb_netconf_request(
    /// [in] service descriptor
    RwUAgent_Service* srv,
    /// [in] the request to forward
    const NetconfReq* req,
    /// [in] Instance pointer
    void* vinstance,
    /// [in] the request closure, indicates how to respond
    NetconfRsp_Closure rsp_closure,
    /// [in] the closure data, mus hand back to rwmsg
    void* closure_data
  );

  /// Received a forward request from messaging.
  void netconf_request(
    /// [in] The request
    const NetconfReq* req,
    /// [in] The rwmsg closure
    NetconfRsp_Closure rsp_closure,
    /// [in] The rwmsg closure data
    void* closure_data
  );

private:

  //! The owning uAgent.
  Instance* instance_ = nullptr;

  //! rwmemlog logging buffer
  RwMemlogBuffer memlog_buf_;

  /// The messaging endpoint for the uAgent
  rwmsg_endpoint_t *endpoint_ = nullptr;

  /// The server channel (and, effectively, the execution context)
  rwmsg_srvchan_t *srvchan_ = nullptr;

  /// The uAgent service descriptor
  RwUAgent_Service rwua_srv_;
};


/*!
 * RW.uAgent instance.  Responsible for:
 *  - holding tasklet instance data
 *  - holding messaging server and endpoint data
 *  - accepting new connections (thus creating a NbReq)
 *  - instantiating a MsgClient (thus creating the one-and-only
 *    internal-massaging based client)
 *  - managing Requests on behalf of Clients
 * - statistics
 *  - ATTN: maintaining a DOM
 *  - ATTN: responding directly to DOM-get requests
 */
class Instance
{
public:
  Instance(rwuagent_instance_t rwuai);
  ~Instance();

  // cannot copy
  Instance(const Instance&) = delete;
  Instance& operator=(const Instance&) = delete;

public:
  /// Return the rwmemlog instance so child objects can allocate their own buffers
  rwmemlog_instance_t* get_memlog_inst();
  /// Return the rwmemlog buffer so callbacks can log with the instance's buffer
  rwmemlog_buffer_t* get_memlog_buf();
  rwmemlog_buffer_t** get_memlog_ptr();

public:

  typedef std::list<std::string> modules_t;
  typedef std::unordered_map<rwsched_CFSocketRef,rwsched_CFRunLoopSourceRef> cf_src_map_t;

public:
  /// Get the tasklet info instance
  rwtasklet_info_ptr_t rwtasklet();

  /// Get the rwsched instance
  rwsched_instance_ptr_t rwsched();

  /// Get the rwsched instance
  rwsched_tasklet_ptr_t rwsched_tasklet();

  /// Get the rwtrace instance
  rwtrace_ctx_t* rwtrace();

  /// Get the rwlog instance
  rwlog_ctx_t* rwlog();

  /// Get the rwvcs instance
  struct rwvcs_instance_s* rwvcs();

  //! Get the DTS API handle.
  DtsMember* dts() const noexcept
  {
    return dts_;
  }

  //! Get the DTS API handle.
  rwdts_api_t* dts_api() const noexcept
  {
    auto api = dts_->api();
    RW_ASSERT(api);
    return api;
  }

  //! Get the current schema.
  const rw_yang_pb_schema_t* ypbc_schema() const noexcept
  {
    return ypbc_schema_;
  }

  //! Get the XML manager.
  rw_yang::XMLManager* xml_mgr() const noexcept
  {
    return xml_mgr_.get();
  }

  //! Get the yang model.
  rw_yang::YangModel* yang_model() const noexcept
  {
    return xml_mgr_->get_yang_model();
  }

  // ATTN: This needs to go into another object?
  //! Get the configuration DOM.
  rw_yang::XMLDocument* dom() const noexcept
  {
    return dom_.get();
  }

  //! Determine if the DTS member API is ready for queries.
  bool dts_ready() const noexcept
  {
    return dts_ && dts_->is_ready();
  }

  //! Get the pointer to the uagent concurrent dispatch queue.
  rwsched_dispatch_queue_t cc_dispatchq() const noexcept
  {
    return cc_dispatchq_;
  }

  StartupHandler* startup_hndl() const noexcept
  {
    return startup_handler_.get();
  }

  /*!
   * Start the instance. All the startup initializations
   * are asynchronously dispatched to async_start
   * function.
   */
  void start();

  /*!
   * Asynchronously invoked by the start() member
   * function. Creates messaging endpoints, DTS
   * registrations, confd connection, et cetera...
   */
  static void async_start(void* ctxt);

  static void async_start_dts(void* ctxt);

  static void async_start_confd(void* ctxt);

  /*!
   * Register the nodes from the new schema
   * after dynamic schema loading
   */
  static void dyn_schema_dts_registration(
            void *ctxt);

  /// Initialize the rwmsg service servers
  void start_msg();

  /// setup the DOM from the modules
  void setup_dom(const char *module_name);

  /// Reloads the dom with the new module included
  rw_yang::YangModule* load_module(const char *module_name);

  /// Start Confd CDB upgrade
  void start_upgrade(size_t n_modules, char** module_name);

  rw_status_t handle_dynamic_schema_update(const int batch_size,
                                           const char * const * module_names,
                                           const char * const * so_filenames);

  rw_status_t perform_dynamic_schema_update();

  // Update dynamic schema loading state
  void update_dyn_state(RwMgmtSchema_ApplicationState state,
                        const std::string& str);

  // Update dynamic schema loading state with err string set to ' ' (space)
  void update_dyn_state(RwMgmtSchema_ApplicationState state);

  /// Connect to a confd server. Does the data provider registrations
  /// and sets up notification socket
  rw_status_t setup_confd_connection();

  /// Does the config subscription with confd
  rw_status_t setup_confd_subscription();

  /// Start the confd configuration reload asynchronously
  void start_confd_reload();

  /// tries to connect to a confd server using setup_confd_connection
  void try_confd_connection();

  // Close a CF socket
  void close_cf_socket(rwsched_CFSocketRef s);

  /// Annotate the YANG model with tailf hash values
  void annotate_yang_model_confd();

  /*!
   * Update uagent statistics
   */
  void update_stats (RwMgmtagt_SbReqType type,
                     const char *req,
                     RWPB_T_MSG(RwMgmtagt_SpecificStatistics_ProcessingTimes) *sbreq_stats);
private:
  rw_status_t fill_in_confd_info();

private:
  static const size_t MEMORY_LOGGER_INITIAL_POOL_SIZE = 12ul;
  RwMemlogInstance memlog_inst_;
  RwMemlogBuffer memlog_buf_;

public: // ATTN: private

  // ATTN: Is this really needed?  It is in tasklet
  std::string instance_name_;

  // ATTN: Why public?
  /// Dynamic schema loading state
  struct schema_state {
    RwMgmtSchema_ApplicationState state;
    std::string err_string;
  };

  schema_state schema_state_{RW_MGMT_SCHEMA_APPLICATION_STATE_INITIALIZING, " "};

  /// The plugin component and instance
  rwuagent_instance_t rwuai_;

  /// The list of loaded modes (Not strictly necesary?)
  modules_t modules_;

public: // ATTN: private
  /// The Confd Port to connect to
  in_port_t confd_port_ = 0;

  /// Confd IPC Unix domain socket
  std::string confd_unix_socket_;

  struct sockaddr_un confd_unix_addr_;
  struct sockaddr_in confd_inet_addr_;

  struct sockaddr *confd_addr_ = nullptr;
  size_t confd_addr_size_ = 0;

  // schema which needs to be loaded
  std::string yang_schema_;

  // ATTN: move to confd daemon
  /// Confd Configuration handler
  NbReqConfdConfig *confd_config_ = nullptr;

  /// Confd Deamon that supports data retrieval and actions
  ConfdDaemon *confd_daemon_ = nullptr;

  // ATTN: move to confd daemon
  /// The number of attempts to connect to confd
  uint8_t confd_connection_attempts_ = 0;

  /// A mapping from RW-sched assigned handles to sources
  cf_src_map_t cf_srcs_;

  // Each type has its own stats, and CommandStats
  typedef std::unique_ptr<OperationalStats> op_stats_uptr_t;
  std::array <op_stats_uptr_t, RW_MGMTAGT_SB_REQ_TYPE_MAXIMUM> statistics_;

  /// Stores the last error encountered by uagent and returns it when requested
  std::string last_error_;

  ConfdUpgradeContext upgrade_ctxt_;

  bool unique_ws_ = false;
  std::string mgmt_workspace_;

  ///
  uint32_t cli_dom_refresh_period_msec_ = RWUAGENT_DOM_CLI_TIMER_PERIOD_MSEC;
  uint32_t nc_rest_refresh_period_msec_ = RWUAGENT_DOM_NC_REST_TIMER_PERIOD_MSEC;

  // ATTN: Belongs in driver?
  /// List of modules which needs to be loaded for dynamic
  // schema update.
  std::list<std::pair<std::string, std::string>> pending_schema_modules_;
  std::atomic<bool> initializing_composite_schema_;
private:

  /// XML document factory manager
  rw_yang::XMLManager::uptr_t xml_mgr_;

  /// The configuration DOM
  rw_yang::XMLDocument::uptr_t dom_;

  const rw_yang_pb_schema_t* ypbc_schema_ = nullptr;

  /// DTS API manager
  DtsMember* dts_ = nullptr;

  /// Messaging client
  MsgClient* msg_client_ = nullptr;

  std::unique_ptr<DynamicSchemaDriver> schema_driver_ = nullptr;

  /// A concurrent dispatch queue for multi-threading
  rwsched_dispatch_queue_t cc_dispatchq_;

  /// Serial dispatch queue for loading schema files
  rwsched_dispatch_queue_t schema_load_q_;

  ///
  std::unique_ptr<StartupHandler> startup_handler_ = nullptr;

  /// A list used to hold temporary model objects
  std::vector<std::unique_ptr<rw_yang::YangModelNcx>> tmp_models_;
};

/*!
 * ProtoSplitter is a XMLVisitor object that checks whether a node is at a level
 * of application subscription, and if it is, adds it to the a transaction as
 * a query.
 *
 * The ProtoSplitter provides an intermediate solution on applying a DOM of
 * configuration changes to Riftware components. The components are currently
 * implemented to process a CLI line worth of changes in one message.
 *
 * The eventual solution will be similar, the only difference being that the
 * nodes at which the split is done will be the highest node at which an
 * application has registered.
 *
 * This might have been better fit in the XML files, but dts files cannot be
 * accessed due to dependency issues.Also, this functionality is required only
 * in the management agent
 */

class ProtoSplitter
    :public rw_yang::XMLVisitor
{
public:
  /// The transaction to which protos are to be added
  rwdts_xact_t* xact_ = nullptr;
  /// Atleast one message was created
  bool valid_ = false;
  /// A flag to be set on the next DTS transaction
  RWDtsFlag dts_flags_ = RWDTS_FLAG_NONE;

public:

  ProtoSplitter(
    /** Transaction to which the XML is to be split */
    rwdts_xact_t *xact,
    /** [in] the underlying YANG model for the splitter */
    rw_yang::YangModel *model,
    RWDtsFlag flag,
    /** [in] tag of the extension to split at */
    rw_yang::XMLNode* root_node
  );

  /// trivial  destructor
  virtual ~ProtoSplitter() {}

  // Cannot copy
  ProtoSplitter(const ProtoSplitter&) = delete;
  ProtoSplitter& operator=(const ProtoSplitter&) = delete;

public:

  ///@see XMLVisitor::visit
  rw_yang::rw_xml_next_action_t visit(rw_yang::XMLNode* node,
                                      rw_yang::XMLNode** path,
                                      int32_t level);
private:
  rw_yang::XMLNode* root_node_ = nullptr;
};

/*!
 * The ConfigPublishRegistrar helps the uagent register with DTS to provide
 * configuration data for DTS members. Once a configuration transaction
 * is committed by DTS, the management agent walks that data and registers
 * to provide data at different registration points.
 */
class ConfigPublishRegistrar
    :public rw_yang::XMLVisitor
{

 public:
  /** The app data type created from
     the namespace and tag. XML is split
     at nodes which have this app data*/
  rw_yang::AppDataToken<const char*> app_data_;

  /** The API handle at which registrations have to be made */
  Instance *instance_;

 public:
  ConfigPublishRegistrar (
      /** [in] The API handle at which registrations have to be made */
      Instance *inst,
      /** [in] the underlying YANG model for the splitter */
      rw_yang::YangModel *model,
      /** [in] name space of the extension to split at */
      const char *ns,
      /** [in] tag of the extension to split at */
      const char *extension
  );

  virtual ~ConfigPublishRegistrar() {};

  ConfigPublishRegistrar(const ConfigPublishRegistrar&) = delete;
  ConfigPublishRegistrar& operator = (const ConfigPublishRegistrar&) = delete;

 public:
  ///@see XMLVisitor::visit
  rw_yang::rw_xml_next_action_t visit(rw_yang::XMLNode* node,
                                      rw_yang::XMLNode** path,
                                      int32_t level);
};
}
__BEGIN_DECLS

/*!
 * Compatibility structure for C plugin interface.
 */
struct rwuagent_instance_s {
  CFRuntimeBase _base;
  rwuagent_component_t component;
  rw_uagent::Instance* instance;
  rwtasklet_info_ptr_t rwtasklet_info;
};

__END_DECLS


namespace rw_uagent {

inline rwtasklet_info_ptr_t Instance::rwtasklet()
{
  RW_ASSERT(rwuai_ && rwuai_->rwtasklet_info);
  return rwuai_->rwtasklet_info;
}

inline rwsched_instance_ptr_t Instance::rwsched()
{
  rwtasklet_info_ptr_t tasklet = rwtasklet();
  RW_ASSERT(tasklet->rwsched_instance);
  return tasklet->rwsched_instance;
}

inline rwsched_tasklet_ptr_t Instance::rwsched_tasklet()
{
  rwtasklet_info_ptr_t tasklet = rwtasklet();
  RW_ASSERT(tasklet->rwsched_tasklet_info);
  return tasklet->rwsched_tasklet_info;
}

inline rwtrace_ctx_t* Instance::rwtrace()
{
  rwtasklet_info_ptr_t tasklet = rwtasklet();
  RW_ASSERT(tasklet->rwtrace_instance);
  return tasklet->rwtrace_instance;
}

inline rwlog_ctx_t* Instance::rwlog()
{
  rwtasklet_info_ptr_t tasklet = rwtasklet();
  RW_ASSERT(tasklet->rwlog_instance);
  return tasklet->rwlog_instance;
}

inline struct rwvcs_instance_s* Instance::rwvcs()
{
  rwtasklet_info_ptr_t tasklet = rwtasklet();
  RW_ASSERT(tasklet->rwvcs);
  return tasklet->rwvcs;
}

inline rwmemlog_instance_t * Instance::get_memlog_inst()
{
  return memlog_inst_;
}

inline rwmemlog_buffer_t * Instance::get_memlog_buf()
{
  return memlog_buf_;
}

inline rwmemlog_buffer_t** Instance::get_memlog_ptr()
{
  return &memlog_buf_;
}

inline 
void Instance::update_dyn_state(RwMgmtSchema_ApplicationState state,
                                const std::string& str)
{
  schema_state_.state = state;
  schema_state_.err_string = str;
}

inline
void Instance::update_dyn_state(RwMgmtSchema_ApplicationState state)
{
  // ATTN: Single space?
  update_dyn_state(state, " ");
}

} // end namespace rwuagent

#endif // CORE_MGMT_RWUAGENT_HPP_
