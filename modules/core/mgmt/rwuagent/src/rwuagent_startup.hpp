
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/*
 * @file rwuagent_startup.hpp
 *
 * Management agent startup phase handler
 */

#ifndef CORE_MGMT_RWUAGENT_STARTUP_HPP_
#define CORE_MGMT_RWUAGENT_STARTUP_HPP_

#if __cplusplus
#if __cplusplus < 201103L
#error "Requires C++11"
#endif
#endif

#include "rwuagent.h"
#include <rwmemlog.h>
#include <rwmemlog_mgmt.h>
#include <rwmemlogdts.h>

namespace rw_uagent {

static const char* CONFD_INIT_FILE = ".init_complete";
static const uint64_t CRITICAL_TASKLETS_WAIT_TIME = NSEC_PER_SEC * 60 * 5; // 5 Mins

//Fwd decl
class Instance;

/*!
 * Management agent startup phase handler, base class. This is
 * a pure virtual class. Concrete implementations exist for Confd
 * and libnetconf servers.
 * An instance of the concrete implementation will be created and 
 * owned by the rwuagent Instance class. This instance will be used 
 * to make the confd/libnetconf daemon progress through its startup/
 * initialization phase till it opens up its northbound interfaces 
 * (NETCONF/CLI/REST etc).
 *
 * The need for such a startup phase manager is to make the bootup of
 * of management agent in sync with the Confd/libnetconf server 
 * startup.
 *
 * In case of Confd, it would be started in phase-0 and would be 
 * moved to next phase as and when management agent proceeds through
 * its startup phase.
 */
class StartupHandler 
{
public:
  StartupHandler(Instance*);

  /// This class and its derived classes would be
  // non copyable and non assignable
  StartupHandler(const StartupHandler& other) = delete;
  void operator=(const StartupHandler& other) = delete;

  virtual ~StartupHandler() = default;

public:
  /*!
   * Useful for doing init connections with the
   * server or for some initialization steps
   */
  virtual rw_status_t initialize_conn() = 0;

  bool is_instance_ready() const noexcept {
    return inst_ready_;
  }

public:
  /*!
   * Calls the correct callback as per the current state
   * of the state machine.
   * State machine is managed and implemented by the
   * concrete implementation.
   */
  virtual void proceed_to_next_state() = 0;

public: // FIX THE INTERFACE!!

  /*!
   * Waits for the critical tasklets to come into Running
   * state before proceeding with the startup state machine.
   * It waits for 5 mins(default) before getting timed out
   * on waiting for critical tasklets.
   */
  virtual rw_status_t wait_for_critical_tasklets();
  /*!
   * Starts the configuration management process via
   * VCS by making an RPC call via DTS.
   */
  virtual void start_mgmt_instance();

  /*!
   * Receives start callback and verifies that it worked.
   */
  virtual void config_mgmt_start(
    rwdts_xact_t*        xact,
    rwdts_xact_status_t* xact_status);

  /*!
   * Creates a configuration Item in the agent
   * config DOM for vstart RPC to work.
   */
  virtual void create_proxy_manifest_config() = 0;

  /*!
   * Get the management component name.
   */
  virtual const char* get_component_name() = 0;

  /*!
   * Get the log file paths for the respective
   * management server instance.
   */
  virtual std::vector<std::string> get_log_files() = 0;

protected:
  Instance* instance_ = nullptr;
  //! rwmemlog logging buffer
  RwMemlogBuffer memlog_buf_;
  void* cb_data_ = nullptr;
  bool inst_ready_ = false;

protected:
  /*!
   * Called once the criticals tasklets are ready
   * OR when the timer gets timed out waiting for the
   * critical tasklets to come up.
   */
  virtual void tasks_ready();
 
private:
  static void tasks_ready_cb(void* ctx);

  // Timer related functions
  static void tasks_ready_timer_expire_cb(void* ctx);
  void start_tasks_ready_timer();
  void tasks_ready_timer_expire();
  void stop_tasks_ready_timer();

  // Critical tasks timer
  rwsched_dispatch_source_t tasks_ready_timer_ = nullptr;
};



/*!
 * Startup handler specialized for Confd interaction.
 * It makes use of MAAPI API's to talk with confd and
 * to monitor its progress.
 * Confd startup is divided into multiple phases:
 * 1. Phase-0:
 *    This is the phase in which Confd will be started.
 *    Once initialized in this phase, uAgent can start
 *    with data provider/notification registrations.
 *
 * 2. Phase-1:
 *    Once uAgent is done with doing the registrations and
 *    setting up the socket connections, it will signal 
 *    the Confd daemon to proceed to Phase-1. In this
 *    phase, Confd basically initializes the CDB.
 *    Once Confd is done initializing this phase, uAgent can
 *    now add CDB subscribers.
 *    It will immidiately signal Confd to proceed to phase-2.
 *
 * 3. Phase-2:
 *    In this phase, Confd will bind and start listening to
 *    NETCONF, CLI, REST etc addresses / ports.
 *    After this, the management system can be said to be
 *    ready for accepting traffic.
 *
 */
class ConfdStartupHandler final: public StartupHandler 
{
public:
  ConfdStartupHandler(Instance* instance);

  ~ConfdStartupHandler() {
    close(sock_);
    close(read_sock_);
  }

public:
  using CB = void(*)(void*);

  rw_status_t initialize_conn() override;

public:
  void proceed_to_next_state() override;

  void create_proxy_manifest_config() override;
  const char* get_component_name() override
  {
    return "confd";
  }

  std::vector<std::string> get_log_files() override;

private:
  enum confd_phase_t {
    PHASE_0 = 0,
    PHASE_1,
    PHASE_2,
    RELOAD,
    DONE,
    TRANSITIONING, // in between phase change
  };

  void close_sockets();
  void retry_phase_cb(CB cb);

  // State-1 callback
  static void start_confd_phase_1_cb(void* ctx);
  void start_confd_phase_1();

  // State-2 callback
  static void start_confd_phase_2_cb(void* ctx);
  void start_confd_phase_2();

  // State-3 callback
  static void start_confd_reload_cb(void* ctx);
  void start_confd_reload();

  confd_phase_t curr_phase() const noexcept {
    return state_.first;
  }
  confd_phase_t next_phase() const noexcept {
    return state_.second;
  }

private:
  // Callback function pointer
  typedef void (*MFP)(void*);

  // pair.first = curr_state
  // pair.second = next_state
  using State = std::pair<confd_phase_t, confd_phase_t>;
  State state_{PHASE_0, PHASE_1};
  std::map<State, MFP> state_mc_;

  int sock_ = -1;
  int read_sock_ = -1;

  std::string confd_dir_;

  //ATTN: dont be lazy, create protobuf
  constexpr static const char* manifest_cfg_ = 
                                    "<root>"
                                    "<rw-manifest:manifest xmlns:rw-manifest=\"http://riftio.com/ns/riftware-1.0/rw-manifest\">"
                                    "<rw-manifest:inventory>"
                                    "<rw-manifest:component>"
                                    "<rw-manifest:component-name>confd</rw-manifest:component-name>"
                                    "<rw-manifest:component-type>PROC</rw-manifest:component-type>"
                                    "<rwvcstypes:native-proc xmlns:rwvcstypes=\"http://riftio.com/ns/riftware-1.0/rw-manifest\">"
                                    "<rwvcstypes:exe-path>./usr/bin/rw_confd</rwvcstypes:exe-path>"
                                    "<rwvcstypes:args>--unique $</rwvcstypes:args>"
                                    "</rwvcstypes:native-proc>"
                                    "</rw-manifest:component>"
                                    "</rw-manifest:inventory>"
                                    "</rw-manifest:manifest>"
                                    "</root>";
};



/*!
 * TODO:
 */
class NetconfStartupHandler: public StartupHandler 
{
};

}

#endif
