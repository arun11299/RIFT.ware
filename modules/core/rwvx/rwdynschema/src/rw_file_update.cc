/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 * Creation Date: 12/10/2015
 * 
 */

#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <random>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>

#include "rwyangutil.h"

#include "rw_file_update.h"

using namespace rw_dyn_schema;
using namespace rw_yang;
namespace fs = boost::filesystem;

#define CREATE_STATE(S1, S2, Func)                                      \
  std::make_pair(std::make_pair(S1, S2), &FileUpdateProtocol::Func)

#define ADD_STATE(S1, S2, Func)                 \
  state_mc_.insert(                             \
      CREATE_STATE(S1, S2, Func))


static constexpr const auto ten_seconds = 10LL;
static constexpr const auto thirty_seconds = 30LL;

void rw_run_file_update_protocol(rwsched_instance_ptr_t sched,
                                 rwtasklet_info_ptr_t tinfo,
                                 rwsched_tasklet_ptr_t tasklet,
                                 rwdynschema_dynamic_schema_registration_t* app_data)
{
  FileUpdateProtocol::run(sched, tinfo, tasklet, app_data);
}



bool rw_create_runtime_schema_dir()
{
  std::string rift_root = getenv("RIFT_INSTALL");
  if (!rift_root.length()) {
    rift_root = "/";
  }

  std::string cmd;
  cmd = rift_root + FILE_PROTO_OPS_EXE;

  cmd = cmd + " --lock-file-create --create-schema-dir --lock-file-delete";

  auto ret = std::system(cmd.c_str());
  if (ret != EXIT_SUCCESS) {
    return false;
  }
  return true;
}

// Generates a random number from 1 to 5
static uint8_t get_random_number()
{
  std::default_random_engine re;
  re.seed(std::time(nullptr));
  std::uniform_int_distribution<uint8_t> rnd(1, 5);

  return rnd(re);
}

static int get_inode_no(const char* file)
{
  struct stat var;
  auto ret = stat(file, &var);

  if (ret < 0) {
    return -1;
  }
  return var.st_ino;
}

static void update_lck_timestamp_cb(rwsched_CFRunLoopTimerRef cftimer,
                                    void* user_ctxt)
{
  auto* owning_app = reinterpret_cast<owning_app_info*>(user_ctxt);
  UNUSED(cftimer);
  try{
    fs::last_write_time(owning_app->lock_file(), std::time(nullptr));
  } catch(const fs::filesystem_error & e) {
    // file doesn't exist
  }
}

static bool check_version_staleness()
{
  auto curr_ver = ConfdUpgradeMgr().get_max_version_unlinked();
  if (curr_ver == 0) {
    // No version directory created yet
    return true;
  }
  std::ostringstream strm;
  strm << std::setfill('0') << std::setw(8) << std::hex << curr_ver;

  auto last_write_timer = fs::last_write_time(SCHEMA_VER_DIR + strm.str());
  auto now = std::time(nullptr);

  return (now - last_write_timer) >= STALENESS_THRESH;
}

static void rerun_state_machine(void* ctxt)
{
  auto* self = reinterpret_cast<FileUpdateProtocol*>(ctxt);
  self->reset_state_machine();
}

bool FileUpdateProtocol::create_new_version_dir()
{
  auto ret = execute_cmd("--version-dir-create");
  if (!ret) {
    RW_LOG(this, Error, "Error while creating new version directory");
    return false;
  }
  RW_LOG(this, Info, "Created version directory successfully");
  return true;
}


inline
FileUpdateProtocol::FileUpdateProtocol(rwsched_instance_ptr_t sched,
                                       rwtasklet_info_ptr_t tinfo,
                                       rwsched_tasklet_ptr_t tasklet,
                                       rwdynschema_dynamic_schema_registration_t* app_data)
    :
    instance_name_(tinfo->identity.rwtasklet_name)
    , sched_(sched)
    , tinfo_(tinfo)
    , tasklet_(tasklet)
    , app_data_(app_data)
{
  initialize_state_mc();
  char hostname[MAX_HOSTNAME_SZ];
  hostname[MAX_HOSTNAME_SZ - 1] = 0;
  int res = gethostname(hostname, MAX_HOSTNAME_SZ - 2);
  RW_ASSERT(res != -1);

  lock_file_ = SCHEMA_LOCK_FILE + std::string(hostname);

  const char* rift_root = getenv("RIFT_INSTALL");
  if (nullptr == rift_root) {
    rift_root_ = "/";
  } else {
    rift_root_ = rift_root;
  }

  cmd_ = rift_root_ + "/usr/bin/rwyangutil ";
}

void FileUpdateProtocol::initialize_state_mc()
{
  RW_LOG(this, Info, "Initializing state machine");
  ADD_STATE(STARTED,     CREAT_LOCK,  try_create_lock_file);
  ADD_STATE(CREAT_LOCK,  COPY_TO_TMP, copy_files_to_local_tmp_dir);
  ADD_STATE(COPY_TO_TMP, COPY_TO_REG, copy_files_to_schema_dir);
  ADD_STATE(COPY_TO_REG, CLEANUP,     cleanup_files);
  ADD_STATE(INVALID,     INVALID,     fini_state);
}

void FileUpdateProtocol::run(rwsched_instance_ptr_t sched,
                             rwtasklet_info_ptr_t tinfo,
                             rwsched_tasklet_ptr_t tasklet,
                             rwdynschema_dynamic_schema_registration_t* app_data)
{
  auto* self_ptr = new FileUpdateProtocol(sched, tinfo, tasklet, app_data);
  self_ptr->run_state_machine();

}

bool FileUpdateProtocol::execute_cmd(const std::string& cmd_opt)
{
  const auto& cmd = cmd_ + cmd_opt;
  auto ret = std::system(cmd.c_str());
  if (ret != EXIT_SUCCESS) {
    return false;
  }
  return true;
}

void FileUpdateProtocol::run_state_machine()
{
  while (state_ != std::make_pair(INVALID, INVALID)) {
    auto func = state_mc_[state_];
    (this->*func)();
  }

  if (!owner_.get())
  {
    // If the current process could not own the lock 
    // file, start watching it for liveness and
    // hook up for receiving notification when lock file
    // gets deleted.
    watcher_.reset(new watcher_app_info(this));
    if (!watcher_->start_watching_lock_file()) {
      RW_LOG(this, CritInfo, "Watcher detected lock file staleness");
      auto when = dispatch_time(DISPATCH_TIME_NOW, 0 * NSEC_PER_SEC);
      rwsched_dispatch_after_f(tasklet_,
                               when,
                               rwsched_dispatch_get_main_queue(sched_),
                               this,
                               rerun_state_machine);
    }
  } else {
    app_callback();
  }
}

void FileUpdateProtocol::reset_state_machine()
{
  RW_LOG(this, CritInfo, "Resetting state machine");
  auto sched = sched_;
  auto tasklet = tasklet_;
  auto tinfo = tinfo_;
  auto app_data = app_data_;

  this->~FileUpdateProtocol();
  new(this) FileUpdateProtocol(sched, tinfo, tasklet, app_data);
  this->run_state_machine();
}


void FileUpdateProtocol::fini_state()
{
  RW_LOG(this, Debug, "fini state");
  delete this;
}


FileUpdateProtocol::~FileUpdateProtocol()
{
  owner_.reset();
  watcher_.reset();
}


void FileUpdateProtocol::try_create_lock_file()
{
  // ATTN: When running in collapsed mode or for that matter even
  // in expanded mode, its possible that different tasklets callback gets invoked
  // at different times, worst case one after the other. In such a case
  // all tasklets will be successfull to do an upgrade.
  // Workaround is to check the freshness of the maximum version
  // directory. If its access/write time is less than STALENESS threshold
  // then bail out of this state machine.
  RW_LOG(this, Debug, "try_create_lock_file")

  if (!check_version_staleness()) {
    RW_LOG(this, Info, "Version directory found is new, nothing to be done");
    set_state(INVALID, INVALID);
    // create a dummy owner so as to not rerun
    // the state machine again.
    owner_.reset(new owning_app_info(this, 0));
    return;
  }

  auto ret = execute_cmd("--lock-file-create");
  if (!ret) {
    RW_LOG(this, Info, "Did not get to create lock file");
    set_state(INVALID, INVALID);
    return;
  }

  std::string log;
  RW_LOG(this, CritInfo, (log=instance_name_ + " got the lock file").c_str());

  owner_.reset(new owning_app_info(this, get_inode_no(lock_file_.c_str())));
  owner_->start_update_timer(sched_, tasklet_);

  set_state(CREAT_LOCK, COPY_TO_TMP);
}


void FileUpdateProtocol::copy_files_to_local_tmp_dir() 
{
  RW_LOG(this, Debug, "copy_files_to_local_tmp_dir");

  if (!owner_->check_lck_file_validity()) {
    reset_state_machine();
    return;
  }

  cleanup_tmp_files();

  // ATTN: This could block for quite some time
  // considering the fact that it may be getting
  // copied over NFS link and NFS _will_ give
  // trouble!
  try {
    for (int i = 0; i < app_data_->batch_size; ++i) {
      std::string mod_name(app_data_->module_names[i]);
      std::string lib_name("lib" + mod_name);
      const auto& dir_file = SCHEMA_TMP_LOC + mod_name;
      const auto& dir_lib = SCHEMA_TMP_LOC + lib_name;
      fs::copy(app_data_->fxs_filenames[i], dir_file + ".fxs");
      fs::copy(app_data_->so_filenames[i],  dir_lib + ".so");
      fs::copy(app_data_->yang_filenames[i], dir_file + ".yang");
    }
  } catch (const fs::filesystem_error& e) {
    RW_LOG(this, Error, ("Copying files failed: " + std::string(e.what())).c_str());
    reset_state_machine();
    return;
  }

  RW_LOG(this, Debug, "copying files to tmp completed");
  set_state(COPY_TO_TMP, COPY_TO_REG);
}

void FileUpdateProtocol::copy_files_to_schema_dir()
{
  RW_LOG(this, Debug, "copy_files_to_schema_dir");

  if (!owner_->check_lck_file_validity()) {
    set_state(INVALID, INVALID);
    return;
  }

  auto ret = execute_cmd("--copy-from-tmp");
  if (!ret) {
    RW_LOG(this, Error, "Filesystem error while copying files to schema directory");
    reset_state_machine();
    return;
  }

  RW_LOG(this, Debug, "copying files to perm completed");
  set_state(COPY_TO_REG, CLEANUP);
}

void FileUpdateProtocol::cleanup_files()
{
  if (!owner_->check_lck_file_validity()) {
    reset_state_machine();
    return;
  }

  cleanup_tmp_files();

  if (!create_new_version_dir()) {
    RW_LOG(this, Error, "Creation of version directory failed");
    reset_state_machine();
    return;
  }

  if (!owner_->delete_lock_file()) {
    RW_LOG(this, Error, "Failed to delete lock file");
    reset_state_machine();
  }
  set_state(INVALID, INVALID);
}

void FileUpdateProtocol::cleanup_tmp_files()
{
  FSHelper::remove_all_files(SCHEMA_TMP_LOC);
}

// -------------------------------------------------------------

bool watcher_app_info::start_watching_lock_file()
{
  RW_LOG(parent_, Debug, "start_watching_lock_file");

  ifd_ = inotify_init();
  RW_ASSERT(ifd_ >= 0);

  wfd_ = inotify_add_watch(ifd_, parent_->lock_file_.c_str(), IN_DELETE);

  if (wfd_ < 0) {
    if (!fs::exists(parent_->lock_file_.c_str())) {
      RW_LOG(parent_, CritInfo, "No lock file present to watch");
      parent_->reset_state_machine();
      return true;
    }
    RW_LOG(parent_, Error, "inotify_add_watch failed");
    return false;
  }

  fcntl(ifd_, F_SETFL, O_NONBLOCK);
  
  if (!create_source()) {
    return false;
  }
  create_timer();

  return true;
}

void watcher_app_info::handle_inotify_read(void *ctxt)
{
  auto* self = reinterpret_cast<watcher_app_info*>(ctxt);
  RW_LOG(self->parent_, Debug, "Got inotify event");
  self->parent_->reset_state_machine();
}

static void monitor_lock_file(rwsched_CFRunLoopTimerRef cftimer, 
                              void *ctxt)
{
  auto* self = reinterpret_cast<watcher_app_info*>(ctxt);
  RW_LOG(self->parent_, Debug, "monitor_lock_file");

  if (!self->monitor_lck_timestamp()) {
    RW_LOG(self->parent_, CritInfo, "Lock file not live");
    self->parent_->reset_state_machine();
  }
}

bool watcher_app_info::monitor_lck_timestamp()
{
  // Someone else may have deleted it
  // on finiding it stale
  if (!fs::exists(lock_file())) {
    return true;
  }
  auto last_write_timer = fs::last_write_time(lock_file());
  auto now = std::time(nullptr);

  if ((now - last_write_timer) >= LIVENESS_THRESH) {
    parent_->execute_cmd("--lock-file-delete");
    return false;
  }

  return true;
}

void watcher_app_info::create_timer()
{
  // Start the timer for updating the timestamp of
  // lock file
  rwsched_CFRunLoopRef runloop = rwsched_tasklet_CFRunLoopGetCurrent(parent_->tasklet());
  rwsched_CFRunLoopTimerContext cf_context = { 0, NULL, NULL, NULL, NULL };
  cf_context.info = this;

  monitor_timer_ = rwsched_tasklet_CFRunLoopTimerCreate(
      parent_->tasklet(),
      kCFAllocatorDefault,
      CFAbsoluteTimeGetCurrent() + ten_seconds + get_random_number(),
      ten_seconds + get_random_number(),
      0,
      0,
      monitor_lock_file,
      &cf_context);

  RW_CF_TYPE_VALIDATE(monitor_timer_, rwsched_CFRunLoopTimerRef);

  rwsched_tasklet_CFRunLoopAddTimer(
      parent_->tasklet(),
      runloop,
      monitor_timer_,
      parent_->sched()->main_cfrunloop_mode);
}


bool watcher_app_info::create_source()
{
  source_ = rwsched_dispatch_source_create(
      parent_->tasklet(),
      RWSCHED_DISPATCH_SOURCE_TYPE_READ,
      ifd_,
      0,
      rwsched_dispatch_get_main_queue(parent_->sched()));

  if (!source_) {
    RW_LOG(parent_, Error, "Failed to create dispatch source for reading inotify event.");
    return false;
  }

  rwsched_dispatch_resume(parent_->tasklet(), source_);
  rwsched_dispatch_set_context(parent_->tasklet(), source_, (void *)this);

  rwsched_dispatch_source_set_event_handler_f(
      parent_->tasklet(),
      source_,
      handle_inotify_read);

  return true;
}

watcher_app_info::~watcher_app_info()
{
  // Cleanup timers and dispatch sources
  if (source_) {
    rwsched_dispatch_release(parent_->tasklet(), source_);
  }
  inotify_rm_watch(ifd_, wfd_);
  close(ifd_);

  if (monitor_timer_ && 
      rwsched_tasklet_CFRunLoopTimerIsValid(parent_->tasklet(), monitor_timer_)) {
    rwsched_tasklet_CFRunLoopTimerRelease(parent_->tasklet(), monitor_timer_);
  }

}

// ----------------------------------------------------------------

bool owning_app_info::check_lck_file_validity()
{
  return lock_inode == get_inode_no(lock_file());
}

bool owning_app_info::delete_lock_file()
{
  return parent_->execute_cmd("--lock-file-delete");
}

owning_app_info::~owning_app_info()
{
  if (update_timer_ && 
      rwsched_tasklet_CFRunLoopTimerIsValid(parent_->tasklet(), update_timer_)) {
    rwsched_tasklet_CFRunLoopTimerRelease(parent_->tasklet(), update_timer_);
  }

}

void owning_app_info::start_update_timer(rwsched_instance_ptr_t sched,
                                         rwsched_tasklet_ptr_t tasklet)
{
  // Start the timer for updating the timestamp of
  // lock file
  rwsched_CFRunLoopRef runloop = rwsched_tasklet_CFRunLoopGetCurrent(tasklet);
  rwsched_CFRunLoopTimerContext cf_context = { 0, NULL, NULL, NULL, NULL };
  cf_context.info = this;

  update_timer_ = rwsched_tasklet_CFRunLoopTimerCreate(
      tasklet,
      kCFAllocatorDefault,
      CFAbsoluteTimeGetCurrent() + ten_seconds + get_random_number(),
      ten_seconds + get_random_number(),
      0,
      0,
      update_lck_timestamp_cb,
      &cf_context);

  RW_CF_TYPE_VALIDATE(update_timer_, rwsched_CFRunLoopTimerRef);

  rwsched_tasklet_CFRunLoopAddTimer(
      tasklet,
      runloop,
      update_timer_,
      sched->main_cfrunloop_mode);
}

