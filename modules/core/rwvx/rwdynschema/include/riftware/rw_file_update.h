/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 * Creation Date: 11/10/2015
 * 
 */

#ifndef RWFILE_UPDATE_H__
#define RWFILE_UPDATE_H__

#include <rwtasklet.h>

#if defined(__cplusplus)
#include <string>
#include <map>
#include "rw_confd_upgrade.hpp"
extern "C" {
#endif

#include <sys/stat.h>
#include <sys/inotify.h>
#include "rwlog.h"
#include "rwdynschema.h"
#include "rw-dynschema-log.pb-c.h"
#include "rwyangutil.h"

#define RW_MA_Paste3(a,b,c) a##_##b##_##c

#define RW_LOG(__instance__, evvtt, ...)                                \
  RW_MA_INST_LOG_Step1(__instance__, RW_MA_Paste3(RwDynschemaLog,notif,evvtt), __VA_ARGS__)
    
#define RW_MA_INST_LOG_Step1(__instance__, evvtt, ...)                  \
  RWLOG_EVENT((__instance__)->rwlog(), evvtt, (uint64_t)__instance__, __VA_ARGS__)


#ifdef __cplusplus

namespace rw_dyn_schema {

struct owning_app_info;
struct watcher_app_info;

class FileUpdateProtocol
{
  /// State during protocol update
  enum State {
    STARTED = 0,
    // Owners state
    CREAT_LOCK,
    COPY_TO_TMP,
    COPY_TO_REG,
    CLEANUP,
    // Watchers state
    POLLING,
    LOCK_REMOVED,
    // 
    INVALID,
  };

  FileUpdateProtocol(const FileUpdateProtocol& other) = delete;
  void operator=(const FileUpdateProtocol& other) = delete;

  FileUpdateProtocol(rwsched_instance_ptr_t sched,
                     rwtasklet_info_ptr_t tinfo,
                     rwsched_tasklet_ptr_t tasklet,
                     rwdynschema_dynamic_schema_registration_t* app_data);
           

  ~FileUpdateProtocol();

public:

  static void run(rwsched_instance_ptr_t sched,
                  rwtasklet_info_ptr_t tinfo,
                  rwsched_tasklet_ptr_t tasklet,
                  rwdynschema_dynamic_schema_registration_t* app_data);

  // Return the tasklets log instance
  rwlog_ctx_t* rwlog() const noexcept;
  rwsched_tasklet_ptr_t tasklet() const noexcept;
  rwsched_instance_ptr_t sched() const noexcept;

  void app_callback();

private:
  /// State Functions
  void run_state_machine();
  void initialize_state_mc();
  void try_create_lock_file();
  void copy_files_to_local_tmp_dir();
  void copy_files_to_schema_dir();
  void cleanup_files();

private: // Not part of state mc directly

  void cleanup_tmp_files();
  bool create_new_version_dir();

public:
  // Member functions for modifying
  // state machine
  void fini_state();
  void reset_state_machine();
  void set_state(State curr_state, State nxt_state);

public:
  // Executes the shlee command 'rw_file_proto_ops'
  // with the option provided as argument
  bool execute_cmd(const std::string& cmd_opt);

public:

  typedef void (FileUpdateProtocol::*MFP)();
  // pair.first = curr_state
  // pair.second = next_state
  using FState = std::pair<State, State>;

  std::string instance_name_;
  // lock file name
  std::string lock_file_;

  FState state_{STARTED, CREAT_LOCK};
  std::map<FState, MFP> state_mc_;

  std::unique_ptr<owning_app_info> owner_ = nullptr;
  std::unique_ptr<watcher_app_info> watcher_ = nullptr;

  // Tasklet related info
  rwsched_instance_ptr_t sched_;
  rwtasklet_info_ptr_t tinfo_;
  rwsched_tasklet_ptr_t tasklet_;

  // App callback related info
  rwdynschema_dynamic_schema_registration_t* app_data_;

  std::string rift_root_;
  std::string cmd_;
};

/*
 * Composition of data and functions required
 * only by the app owning the lock file
 */
struct owning_app_info {
  owning_app_info(FileUpdateProtocol* parent,
                  int inode): parent_(parent)
                            , lock_inode(inode)
  { RW_ASSERT(lock_inode != -1); }
 
  ~owning_app_info();

  FileUpdateProtocol* parent_ = nullptr;
  int lock_inode = -1;

  bool delete_lock_file();
  bool check_lck_file_validity();
  const char* lock_file() const noexcept;

  rwsched_CFRunLoopTimerRef update_timer_ = 0;
  void start_update_timer(rwsched_instance_ptr_t sched,
                          rwsched_tasklet_ptr_t tasklet);
};

/*
 * Composition of data and functions required
 * only by the app watching the lock file
 */
struct watcher_app_info {
  watcher_app_info(FileUpdateProtocol* parent): parent_(parent)
  {}
  ~watcher_app_info();

  bool start_watching_lock_file();
  // dispatch source related functions
  bool create_source();
  void create_timer();
  bool monitor_lck_timestamp();
  const char* lock_file() const noexcept;

  // IO callback function
  static void handle_inotify_read(void *ctxt);

  int ifd_ = -1;
  int wfd_ = -1;
  FileUpdateProtocol* parent_ = nullptr;
  rwsched_dispatch_source_t source_ = 0;
  rwsched_CFRunLoopTimerRef monitor_timer_ = 0;
};

inline
void FileUpdateProtocol::set_state(State curr_state, State nxt_state)
{
  state_.first = curr_state;
  state_.second = nxt_state;
}

inline
rwlog_ctx_t* FileUpdateProtocol::rwlog() const noexcept
{
  RW_ASSERT(tinfo_->rwlog_instance);
  return tinfo_->rwlog_instance;
}

inline
rwsched_tasklet_ptr_t FileUpdateProtocol::tasklet() const noexcept
{
  return tasklet_;
}

inline
rwsched_instance_ptr_t FileUpdateProtocol::sched() const noexcept
{
  return sched_;
}

inline
void FileUpdateProtocol::app_callback()
{
  RW_LOG(this, Debug, "Calling application callback");
      app_data_->app_sub_cb(app_data_->app_instance,
                            app_data_->batch_size,
                            app_data_->module_names,
                            app_data_->fxs_filenames,
                            app_data_->so_filenames,
                            app_data_->yang_filenames);
  fini_state();
}

inline
const char* owning_app_info::lock_file() const noexcept {
  return parent_->lock_file_.c_str();
}

inline
const char* watcher_app_info::lock_file() const noexcept {
  return parent_->lock_file_.c_str();
}

};

#if defined(__cplusplus)
}
#endif

#endif /* def __cplusplus */
#endif
