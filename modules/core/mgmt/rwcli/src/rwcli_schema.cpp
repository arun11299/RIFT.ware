

/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */



/**
 * @file rwcli_schema.cpp
 * @author Balaji Rajappa (balaji.rajappa@riftio.com)
 * @date 10/14/2015
 * @brief Schema Manager for RW.CLI 
 */

#include <sys/inotify.h>
#include <limits.h>

#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>

#include "rwcli_schema.hpp"
#include "yangncx.hpp"

#define RIFT_INSTALL_ENV "RIFT_INSTALL"

using namespace rw_yang;
namespace fs = boost::filesystem;


// Schema Path relative to rift install
const std::string SCHEMA_PATH("var/rift/schema");

const fs::path YANG_DIR{"yang"};
const fs::path VER_DIR{"version"};
const std::string YANG_CUR_VER_LINK{"confd_yang"};

const unsigned MAX_READ = 16;
const unsigned BUF_LEN = (MAX_READ * (sizeof(struct inotify_event) + NAME_MAX + 1));

SchemaInfo::SchemaInfo(
              const std::string& name, 
              SchemaInfo::SchemaState state,
              timestamp_t ts)
  : name_(name),
    state_(state),
    load_timestamp_(ts)
{
}

SchemaUpdater::SchemaUpdater(SchemaManager* mgr)
  : mgr_(mgr)
{
}

SchemaFileInfo::SchemaFileInfo(const std::string& filename, 
                  SchemaFileInfo::SchemaFileState state)
  : filename_(filename),
    state_(state)
{
}

void SchemaChangeset::add_change(const std::string& filename, uint32_t inotify_mask)
{
  // Currently handle only the create event
  if (inotify_mask & IN_CREATE) {
    schema_files_.emplace_back(filename, SchemaFileInfo::SCHEMA_FILE_CREATED);
  }
}

SchemaManager::SchemaManager()
  : SchemaManager(SCHEMA_PATH)
{
}

SchemaManager::SchemaManager(const std::string& schema_path)
  : schema_path_(schema_path)
{
  model_.reset(YangModelNcx::create_model());
  RW_ASSERT(model_);

  model_->load_module(CONFIG_ROOT_MODEL);

  rift_install_ = getenv(RIFT_INSTALL_ENV);
  if (!rift_install_.length()) {
    rift_install_ = "/";
  }

  load_all_schemas();

  updater_.reset(new InotifySchemaUpdater(this));
}

bool SchemaManager::load_all_schemas()
{
  // Load all yang modules present in the latest schema version
  // directory.
  fs::path schema_path = fs::path(rift_install_) / fs::path(schema_path_);
  fs::path version_dir = schema_path / VER_DIR;
  fs::path confd_yang  = version_dir / fs::path(YANG_CUR_VER_LINK);

  fs::path yang_dir;

  if (fs::exists(confd_yang)) {

    if (!fs::is_symlink(confd_yang)) {
      std::cerr << "confd_yang is not a symlink? " << confd_yang << std::endl;
      return false;
    }

    char buf[4096] = {};
    auto len = readlink(confd_yang.string().c_str(), buf, sizeof(buf) - 1);
    if (len == -1) {
      std::cerr << "Failed to read symbolic link " << confd_yang.string() << std::endl;
      return false;
    }

    boost::system::error_code ec;
    yang_dir = fs::canonical(fs::path(buf), version_dir, ec);

    if (ec) {
      std::cerr << "Failed to create absolute path " << buf << ec.message() << std::endl;
      return false;
    }

    if (fs::exists(yang_dir / YANG_DIR)) {
      yang_dir /= YANG_DIR;
    }

  } else {
    yang_dir = schema_path / YANG_DIR;
  }

  if (!fs::exists(yang_dir)) {
    std::cerr << "Schema Yang directory does not exists " << yang_dir << std::endl;
    return false;
  }

  for ( fs::directory_iterator it(yang_dir); it != fs::directory_iterator(); ++it ) {

    auto& path = it->path();

    if (path.extension().string() == ".yang") {

      std::string module_name = path.stem().string();
      if (load_schema (module_name) != RW_STATUS_SUCCESS) {
        return false;
      }
    }
  }

  return true;
}

rw_status_t SchemaManager::load_schema(const std::string& schema_name)
{
  // Schema already loaded will not be reloaded. A modified schema with the same
  // name will be new revision. Revisions are not yet supported.
  YangModule *module = model_->load_module(schema_name.c_str());
  if (module == nullptr) {
    std::cerr << "Loading yang module " << schema_name << " failed\n";
    return RW_STATUS_FAILURE; 
  }

  timestamp_t ts = std::chrono::system_clock::now();
  auto result = schema_table_.emplace(std::piecewise_construct,
                  std::forward_as_tuple(schema_name), 
                  std::forward_as_tuple(schema_name, 
                    SchemaInfo::SCHEMA_STATE_LOADED, ts));
  if (!result.second) {
    std::cerr << schema_name << " already loaded\n";
  }

  return RW_STATUS_SUCCESS;
}

void SchemaManager::show_schemas(std::ostream& os)
{
  for (auto pair: schema_table_) {
    os << "\t\t" << pair.second.name_ << std::endl; 
  }
}

std::string SchemaManager::get_cli_manifest_dir()
{
  // ATTN:- Fix this when the dynamic schema supports cli manifest files
  return rift_install_ + "/" + schema_path_ + "/cli";
}

InotifySchemaUpdater::InotifySchemaUpdater(SchemaManager* mgr)
  : SchemaUpdater(mgr)
{
  int mask = 0;
  const char* rift_install = getenv(RIFT_INSTALL_ENV);
  fs::path rift_yang_path(rift_install);
  fs::path rift_ver_path(rift_install);

  rift_yang_path /= fs::path(mgr_->schema_path_) / YANG_DIR;
  rift_ver_path /= fs::path(mgr_->schema_path_) / VER_DIR;

  inotify_fd_ = inotify_init1(IN_CLOEXEC);
  RW_ASSERT(inotify_fd_ != -1);

  // Add watchers
  // ATTN: watch for modify when we support yang revisions
  // ATTN: Currently we can handle only CREATE, for DELETE one has to unload the
  // module and we don't have that support yet in the YangModel
  mask = IN_CREATE | IN_DELETE;
  yang_dir_watchfd_ = inotify_add_watch(inotify_fd_, rift_yang_path.c_str(), mask);
  if (yang_dir_watchfd_ == -1) {
    //report an error
    std::cerr << "Inotify add watch for " << rift_yang_path << " failed: " 
              << errno << std::endl;
  }

  // Watch for the delete followed by a create for confd_yang softlink 
  ver_dir_watchfd_ = inotify_add_watch(inotify_fd_, rift_ver_path.c_str(), mask);
  if (ver_dir_watchfd_ == -1) {
    // report an error
    std::cerr << "Inotify add watch for " << rift_ver_path << " failed: " 
              << errno << std::endl;
  }
}

InotifySchemaUpdater::~InotifySchemaUpdater()
{
  inotify_rm_watch(inotify_fd_, yang_dir_watchfd_);
  inotify_rm_watch(inotify_fd_, ver_dir_watchfd_);
  close(inotify_fd_);
}


int InotifySchemaUpdater::get_handle()
{
  return inotify_fd_;
}

int InotifySchemaUpdater::check_for_updates()
{
  char buf[BUF_LEN]; // ATTN: aligned(8)?

  // Read the inotify_event and iterate through the events
  ssize_t nread = read(inotify_fd_, buf, BUF_LEN); 
  if (nread == -1) {
    std::cerr << "Inotify read failed: " << errno << std::endl;
    return 0;
  }

  // There can be more than one inotify event in a single read
  // If the change is related to yang directory then add the file to the
  // changeset.
  // If the change is realted to version directory, then check if a new
  // confd_yang link is created. If so, load the changeset schemas
  for (char* ptr = buf; ptr < buf + nread;) {
    struct inotify_event* event = (struct inotify_event*)ptr;

    if (event->wd == yang_dir_watchfd_) {
      // Interested only in yang files
      fs::path filename(event->name);
      if (filename.extension().compare(".yang") == 0) {
        changeset_.add_change(event->name, event->mask);
      }
    } else if (event->wd == ver_dir_watchfd_) {
      if (YANG_CUR_VER_LINK.compare(event->name) == 0 &&
          (event->mask & IN_CREATE)) {
        load_modules();
        changeset_.clear();
      }
    } else {
      const int invalid_watch_fd = 0;
      RW_ASSERT(invalid_watch_fd);
    }

    ptr += sizeof(struct inotify_event) + event->len;
  }
  return 0;
}

void InotifySchemaUpdater::load_modules()
{
  // Iterate through the schema files and load files that are newly created.
  for (auto schema: changeset_.schema_files_) {
    if (schema.state_ == SchemaFileInfo::SCHEMA_FILE_CREATED) {
      fs::path name(schema.filename_);
      std::cout << "Loading module: " << name.stem() << std::endl;
      mgr_->load_schema(name.stem().c_str());
    }
  }
}

