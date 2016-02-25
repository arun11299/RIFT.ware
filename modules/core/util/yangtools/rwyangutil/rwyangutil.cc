
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
* @file rw_file_proto_ops.cc
* @author Arun Muralidharan
* @date 01/10/2015
* @brief App library file protocol operations
* @details App library file protocol operations
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <functional>
#include <map>
#include <cassert>
#include <chrono>
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "rwyangutil.h"

static const char* RANDOM_SCHEMA_DIR_PREFIX = "./var/rift/.s.";

namespace fs = boost::filesystem;

class CmdParser
{
private:
  const char* const* begin_ = nullptr;
  const char* const* end_ = nullptr;

  struct get_next_state {
    const char* const* tbegin_ = nullptr;
  };
  get_next_state gns_;

public:
  CmdParser(const char* const* argv, size_t argc):
    begin_(argv),
    end_(argv + argc) {}

  CmdParser(const CmdParser& other) = delete;
  void operator=(const CmdParser& other) = delete;

  ~CmdParser() = default; 

public:
  void print_help();
  // Gets the value associated with the option provided
  const char* get_cmd_option(const std::string& option) const noexcept;

  // Gets the first operation/flag/argument specified
  // to this executable
  const char* get_first_operation() const noexcept;

  /// Used for iterating over the options
  // and performing cascaded operations
  const char* get_next_operation() noexcept;

  // Checks if a command line option was provided or not
  bool cmd_option_exists(const std::string& option) const noexcept;
};

void CmdParser::print_help()
{
  std::cout << "Usage: ./rw_file_proto_ops <operation>" << "\n";
  std::cout << "Valid set of operations: " << "\n";
  std::cout << "1. Create lock file:                    --lock-file-create\n";
  std::cout << "2. Delete lock file:                    --lock-file-delete\n";
  std::cout << "3. Create new version directory:        --version-dir-create\n";
  std::cout << "4. Clean temporary files:               --tmp-file-cleanup\n";
  std::cout << "5. Copy files from temporary dir to permanent dir: --copy-from-tmp\n";
  std::cout << "6. Create Schema Directory:             --create-schema-dir\n";
  std::cout << "7. Remove Schema Directory:             --remove-schema-dir\n";
  std::cout << "8. Init Schema Directory:               --init-schema-dir\n";
  std::cout << "9. Prune Schema Directories:            --prune-schema-dir\n";
  std::cout << "10. Remove non unique Confd workspaces: --rm-non-unique-confd-ws\n";
  std::cout << "11. Remove unique Confd workspace:      --rm-unique-confd-ws\n";
  std::cout << "12. Archive Confd persist workspace:    --archive-confd-persist-ws\n";
  std::cout << std::endl;
}

const char* CmdParser::get_cmd_option(const std::string& option) const noexcept
{
  auto itr = std::find(begin_, end_, option);
  if (itr != end_ && ++itr != end_) {
    return *itr;
  }
  return nullptr;
}

const char* CmdParser::get_first_operation() const noexcept
{
  return *begin_;
}

const char* CmdParser::get_next_operation() noexcept
{
  if (nullptr == gns_.tbegin_) {
    gns_.tbegin_ = begin_;
  }
  if (gns_.tbegin_ != end_) {
    return *++(gns_.tbegin_);
  }
  return nullptr;
}

bool CmdParser::cmd_option_exists(const std::string& option) const noexcept
{
  return std::find(begin_, end_, option) != end_;
}

class FileProtoOps
{
 public:

  typedef std::map<std::string, std::function<bool(FileProtoOps*)>> cmd_func_map_t;
  typedef std::map<std::string, const char*> fext_dir_map_t;

  static void init_cmd_map();

  bool execute_cmd(const char* cmd);

  bool validate_cmd(const char* cmd);

  FileProtoOps();

 private:

  bool create_lock_file();

  bool delete_lock_file();

  bool create_new_vesion_dir();

  bool cleanup_tmp_files();

  bool copy_files_from_tmp_to_perm();

  bool create_schema_dir();

  bool remove_schema_dir();

  bool init_schema_dir();

  bool prune_schema_dir();

  bool cleanup_lock_files();

  bool cleanup_excess_version_dirs();

  bool cleanup_stale_version_dirs();

  bool remove_non_unique_confd_ws();

  bool remove_unique_confd_ws();

  // Used internally by remove_non_unique_confd_ws
  // and remove_unique_confd_ws
  bool remove_confd_ws(const char*);

  bool archive_confd_persist_ws();

  std::tuple<unsigned, unsigned> get_max_ver_num_and_count();

  /* File support operations */
  static bool fs_create_directory(const std::string& path);
  static bool fs_create_directories(const std::string& path);
  static bool fs_create_softlinks(const std::string& spath,
                                  const std::string& dpath);
  static bool fs_empty_the_dir(const std::string& dpath);
  static bool fs_remove_directory(const std::string& dpath);
  static std::string fs_read_symlink(const std::string& sym_link);
  static bool fs_create_symlink(const std::string& target,
                                const std::string& link);
  static bool fs_rename(const std::string& old_path,
                        const std::string& new_path);
  static bool fs_remove(const std::string& path);

 private:

  static cmd_func_map_t cmd_map;
  static fext_dir_map_t fext_map;

  std::string rift_root_;
  std::string schema_path_;
  std::string schema_tmp_;
  std::string lock_file_;
  std::string schema_ver_dir_;
  std::string latest_ver_dir_;
  std::string image_schema_path_;
  std::string lock_dir_;
};

FileProtoOps::cmd_func_map_t FileProtoOps::cmd_map;
FileProtoOps::fext_dir_map_t FileProtoOps::fext_map;

void FileProtoOps::init_cmd_map()
{
  // Initialize the cmd to function map.
  cmd_map["--lock-file-create"]        = &FileProtoOps::create_lock_file;
  cmd_map["--lock-file-delete"]        = &FileProtoOps::delete_lock_file;
  cmd_map["--version-dir-create"]      = &FileProtoOps::create_new_vesion_dir;
  cmd_map["--tmp-file-cleanup"]        = &FileProtoOps::cleanup_tmp_files;
  cmd_map["--copy-from-tmp"]           = &FileProtoOps::copy_files_from_tmp_to_perm;
  cmd_map["--create-schema-dir"]       = &FileProtoOps::create_schema_dir;
  cmd_map["--remove-schema-dir"]       = &FileProtoOps::remove_schema_dir;
  cmd_map["--init-schema-dir"]         = &FileProtoOps::init_schema_dir;
  cmd_map["--prune-schema-dir"]        = &FileProtoOps::prune_schema_dir;
  cmd_map["--rm-non-unique-confd-ws"]  = &FileProtoOps::remove_non_unique_confd_ws;
  cmd_map["--rm-unique-confd-ws"]      = &FileProtoOps::remove_unique_confd_ws;
  cmd_map["--archive-confd-persist-ws"]= &FileProtoOps::archive_confd_persist_ws;

  // Initialize the file extension to directory map.
  fext_map[".yang"] = "/yang";
  fext_map[".dsdl"] = "/xml";
  fext_map[".fxs"]  = "/fxs";
  fext_map[".cli.xml"]  = "/cli";
}

bool FileProtoOps::validate_cmd(const char* cmd)
{
  if (cmd_map.find(cmd) != cmd_map.end()) {
    return true;
  }
  return false;
}

bool FileProtoOps::execute_cmd(const char* cmd)
{
  auto cmd_handler = cmd_map.find(cmd);
  if (cmd_handler != cmd_map.end()) {
    return cmd_handler->second(this);
  }

  return false;
}

bool FileProtoOps::fs_create_directory(const std::string& path)
{
  try {
    if (!fs::create_directory(path)) {
      return false;
    }
  } catch (const fs::filesystem_error& e) {
    std::cerr << "Exception while creating directory: "
        << path << " "
        << e.what() << std::endl;
    return false;
  }

  return true;
}

bool FileProtoOps::fs_create_directories(const std::string& path)
{
  try {
    if (!fs::create_directories(path)) {
      std::cerr << "Failed to create directory " << path << std::endl;
      return false;
    }
  } catch (const fs::filesystem_error& e) {
    std::cerr << "Exception while creating directories: "
              << path << " "
              << e.what() << std::endl;
    return false;
  }

  return true;
}

bool FileProtoOps::fs_create_softlinks(const std::string& spath,
                                       const std::string& dpath)
{
  for (fs::directory_iterator file(spath); file != fs::directory_iterator(); ++file) {
    try {
      fs::create_symlink(file->path(), dpath + "/" + file->path().filename().string());
    } catch (const fs::filesystem_error& e) {
      std::cerr << "Exception while creating symbolic link to: "
          << file->path().string() << " "
          << e.what() << std::endl;
      return false;
    }
  }
  return true;
}

bool FileProtoOps::fs_empty_the_dir(const std::string& dpath)
{
  for (fs::directory_iterator file(dpath); file != fs::directory_iterator(); ++file) {
    fs::remove_all(file->path());
  }
  return true;
}

bool FileProtoOps::fs_remove_directory(const std::string& dpath)
{
  try {
    fs::remove_all(dpath);
  } catch (const fs::filesystem_error& e) {
    std::cerr << "Exception while removing the dir: "
        << dpath << " "
        << e.what() << std::endl;
    return false;
  }
  return true;
}

std::string FileProtoOps::fs_read_symlink(const std::string& sym_link)
{
  std::string target;

  try {
    auto tpath = fs::read_symlink(sym_link);
    if (!tpath.empty()) {
      target = tpath.string();
    }
  } catch (const fs::filesystem_error& e) {
    std::cerr << "Exception while reading the symlink: "
        << sym_link << " "
        << e.what() << std::endl;
    return target;
  }

  return target;
}

bool FileProtoOps::fs_create_symlink(const std::string& target,
                                     const std::string& link)
{
  try {
    fs::create_symlink(target, link);
  } catch (const fs::filesystem_error& e) {
    std::cerr << "Exception while creating sym link to "
        << target << " as " << link << " "
        << e.what() << std::endl;
    return false;
  }

  return true;
}

bool FileProtoOps::fs_rename(const std::string& old_path,
                             const std::string& new_path)
{
  try {
    fs::rename(old_path, new_path);
  } catch (const fs::filesystem_error& e) {
    std::cerr << "Exception while renaming " 
        << old_path << " to " << new_path << " "
        << e.what() << std::endl;
    return false;
  }

  return true;
}

bool FileProtoOps::fs_remove(const std::string& path)
{
  try {
    fs::remove(path);
  } catch (const fs::filesystem_error& e) {
    std::cerr << "Exception while removing "
        << path << " "
        << e.what() << std::endl;
    return false;
  }

  return true;
}

FileProtoOps::FileProtoOps()
{
  rift_root_ = getenv("RIFT_INSTALL");
  if (!rift_root_.length()) {
    rift_root_ = "/";
  }

  schema_path_       = rift_root_ + "/" + DYNAMIC_SCHEMA_DIR;
  lock_file_         = rift_root_ + "/" + SCHEMA_LOCK_FILE;
  lock_dir_          = rift_root_ + "/" + SCHEMA_LOCK_DIR;
  schema_ver_dir_    = rift_root_ + "/" + SCHEMA_VER_DIR;
  latest_ver_dir_    = rift_root_ + "/" + LATEST_VER_DIR;
  schema_tmp_        = rift_root_ + "/" + SCHEMA_TMP_LOC;
  image_schema_path_ = rift_root_ + "/" + IMAGE_SCHEMA_DIR;
}

bool FileProtoOps::create_lock_file()
{

  // check liveness
  if (fs::exists(lock_file_)) {
    auto last_write_timer = fs::last_write_time(lock_file_);
    auto now = std::time(nullptr);
    
    if ((now - last_write_timer) >= LIVENESS_THRESH) {
      fs_remove(lock_file_);
    }
  }

  // if parent directory doesn't exist, create it
  if (!fs::exists(lock_dir_)) {
    fs_create_directories(lock_dir_);
    fs::permissions(lock_dir_, fs::all_all);
  }

  auto fd = open(lock_file_.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0760);
  if (fd == -1) {
    std::cerr << "Failed to create lock file " << lock_file_ << "\n";
    return false;
  }

  fs::permissions(lock_file_, fs::all_all | fs::set_uid_on_exe | 
                            fs::set_gid_on_exe | fs::sticky_bit);
  return true;
}

bool FileProtoOps::delete_lock_file()
{
  return fs::remove(lock_file_);
}

std::tuple<unsigned, unsigned> FileProtoOps::get_max_ver_num_and_count()
{
  unsigned max   = 0;
  unsigned count = 0;

  for (fs::directory_iterator it(schema_ver_dir_); it != fs::directory_iterator(); ++it) {
    const auto& name = it->path().string();
    if (!fs::is_directory(name)) {
      continue;
    }

    const auto& version = fs::basename(name);
    if (!std::all_of(version.begin(), version.end(), [](char c) {return std::isxdigit(c);})) {
      continue;
    }

    count++;
    unsigned num = 0;
    std::stringstream strm("0x" + version);
    strm >> std::hex >> num;
    if (num > max) {
      max = num;
    }
  }

  return std::make_tuple(max, count);
}

bool FileProtoOps::create_new_vesion_dir()
{
  unsigned curr_ver = 0, count = 0;
  std::tie (curr_ver, count) = get_max_ver_num_and_count();

  std::ostringstream strm;
  strm << std::setfill('0') << std::setw(8) << std::hex << std::uppercase << (curr_ver + 1);

  std::string ver_path(schema_ver_dir_ + strm.str());
  //std::cout << "Creating version directory: " << ver_path << "\n";

  if (!fs_create_directory(ver_path)) {
    return false;
  }

  fs::permissions(ver_path, fs::owner_all| fs::group_all);

  // make symlink to latest
  (void)fs_remove(latest_ver_dir_);
  if (!fs_create_symlink(ver_path, latest_ver_dir_)) {
    return false;
  }

  std::array<const char*, 4> paths {"/fxs", "/xml", "/lib", "/yang"};
  for (auto path : paths) {
    if (!fs_create_directory(ver_path + path)) {
      return false;
    }
    fs::permissions(ver_path + path, fs::owner_all | fs::group_all);
  }

  // Create symlinks for dynamic schema files which are
  // in var/rift/schema/*
  for (auto path : paths) {
    auto res = fs_create_softlinks(schema_path_ + path, ver_path + path);
    if (!res) {
      return false;
    }
  }

  // Create a stamp file
  auto fd = open((ver_path + "/stamp").c_str(), O_CREAT | O_EXCL | O_WRONLY, 0770);
  if (fd == -1) {
    std::cerr << "Failed to create stamp file\n";
    return false;
  }
  close(fd);
  fs::permissions((ver_path + "/stamp"), fs::owner_all | fs::group_all);

  //std::cout << "Created version directory successfully\n";
  return true;
}

bool FileProtoOps::copy_files_from_tmp_to_perm()
{
  for (fs::directory_iterator file(schema_tmp_);
       file != fs::directory_iterator(); ++file)
  {
    auto ext = file->path().extension().string();
    auto filename = file->path().filename().string();

    std::string dir;

    if (ext == ".fxs") {
      dir = "/fxs/";
    } else if (ext == ".dsdl") {
      dir = "/xml/";
    } else if (ext == ".yang") {
      dir = "/yang/";
    } else if (ext == ".so") {
      dir = "/lib/";
    } else {
      assert(0);
    }

    try {
      fs::copy_file(file->path(), schema_path_ + dir + filename,
          fs::copy_option::overwrite_if_exists);
    } catch (const fs::filesystem_error& exp) {
      std::cerr << "Filesystem error while copying files to schema directory: " 
                << exp.what() << "\n";
      return false;
    }
  }
  
  return true;
}

bool FileProtoOps::cleanup_tmp_files()
{
  if (!fs::exists(schema_tmp_) || !fs::is_directory(schema_tmp_)) {
    return false;
  }

  return fs_empty_the_dir(schema_tmp_);
}

bool FileProtoOps::cleanup_lock_files()
{
  if (!fs::exists(lock_dir_) || !fs::is_directory(lock_dir_)) {
    return false;
  }

  return fs_empty_the_dir(lock_dir_);
}

bool FileProtoOps::remove_schema_dir()
{
  if (!fs::exists(schema_path_)) {
    return true;
  }

  //std::cout << "Removing schema directory " << schema_path_ << std::endl;
  return fs_remove_directory(schema_path_);
}

bool FileProtoOps::create_schema_dir()
{
  bool lock_directory_exists = false;
  if (fs::exists(schema_path_)) {
    /*
     * If there is only one subdirectory of the schema directory it is the lock
     * directory, and the rest of it needs to be created.
    */
    size_t subdirectory_count = 0;
    for (fs::directory_iterator it(schema_path_);
         it != fs::directory_iterator();
         ++it) {
      if (fs::is_directory(it->path())) {
        subdirectory_count++;
      }
    }

    if (subdirectory_count > 1) {
      return true;
    } else {
      lock_directory_exists = true;
    }
  }

  /* Check to make sure the image schema path exists */
  if (!fs::exists(image_schema_path_)) {
    std::cerr << "Image schema path does not exists " << image_schema_path_ << std::endl;
    return false;
  }

  /* Create a random directory, create links to the image schema files, and rename it. */
  auto uuid = boost::uuids::random_generator()();
  std::string suuid = boost::uuids::to_string(uuid);

  suuid.erase(std::remove_if(suuid.begin(), suuid.end(), [](char c) 
                             { if (c == '-') { return true; } return false; }), suuid.end());

  std::ostringstream random_dir;
  random_dir << rift_root_ << "/" << RANDOM_SCHEMA_DIR_PREFIX << suuid;

  std::string random_spath = random_dir.str();

  if (!fs_create_directories(random_spath)) {
    return false;
  }

  fs::permissions(random_spath, fs::all_all); // ATTN:- Fix this.Setting permissions to 777 for now, so that make clean works.

  std::array<std::string, RWDYNSCHEMA_TOP_LEVEL_DIRECTORY_COUNT> paths
  {"/fxs", "/xml", "/lib", "/yang", "/lock", "/tmp", "/version", "/cli"};

  for (auto path : paths) {
    std::string const new_directory = random_spath + path;
    if (!fs_create_directory(new_directory)) {
      fs_remove_directory(random_spath);
      return false;
    }
    fs::permissions(new_directory, fs::all_all);
    if (lock_directory_exists) {
      // make lock file in temporary directory
      std::string const new_lock_file = new_directory + LOCK_FILE_NAME;
      auto fd = open(new_lock_file.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0760);
      if (fd == -1) {
        std::cerr << "Failed to create temporary lock file " << new_lock_file << "\n";
        return false;
      }

      fs::permissions(new_lock_file, fs::all_all | fs::set_uid_on_exe | 
                      fs::set_gid_on_exe | fs::sticky_bit);

    }
  }

  // Create symlinks for the image schema files from /usr/data/yang.
  for ( fs::directory_iterator it(image_schema_path_); it != fs::directory_iterator(); ++it) {

    if (!fs::is_regular_file(it->path()) &&
        !fs::is_symlink(it->path()))  {
      continue;
    }

    std::string ext; 
    fs::path fn = it->path().filename();

    for (; !fn.extension().empty(); fn = fn.stem()) {
      ext = fn.extension().string() + ext;
    }

    auto fd_map = fext_map.find(ext);

    if (fd_map != fext_map.end()) {

      std::string target_path = it->path().string();

      if (fs::is_symlink(it->path())) {

        auto tpath = fs_read_symlink(it->path().string());
        if (!tpath.length()) {
          std::cerr << "Failed to read the sym link " << it->path().string() << std::endl;
          fs_remove_directory(random_spath);
          return false;
        }

        target_path = fs::canonical(tpath, image_schema_path_).string();
      }

      std::string target_dir = random_spath + fd_map->second;
      std::string tsymb_link = target_dir + "/" + it->path().filename().string();

      if (!fs_create_symlink(target_path, tsymb_link)) {
        fs_remove_directory(random_spath);
        return false;
      }
    }
  }

  if (lock_directory_exists) {
    // only move the contents, excluding the /lock directory
  for (auto path : paths) {
    if (path == "/lock") {
      continue;
    }
    std::string const tmp_path = random_spath + path;
    std::string const perm_path = schema_path_ + path;
    if (!fs_rename(tmp_path, perm_path)) {
      fs_remove_directory(random_spath);
    }
  }
  } else {
    if (!fs_rename(random_spath, schema_path_)) {
      fs_remove_directory(random_spath);
    }
  }
  // make latest directory
  if (!fs_create_symlink(schema_path_, latest_ver_dir_)) {
    return false;
  }

  // recursively chmod 777 the schema directory
  fs::directory_iterator dir_end;
  for(fs::recursive_directory_iterator dir(schema_path_), dir_end; dir!=dir_end ;++dir) {
    if (fs::is_directory(dir->path())) {
      fs::permissions(dir->path(), fs::all_all);
    }
  }

  if (!fs::exists(schema_path_) || !fs::is_directory(schema_path_)) {
    std::cerr << "Failed to create schema directory " << schema_path_ << std::endl;
    return false;
  }

  //std::cout << "Schema directory " << schema_path_ << " created" << std::endl;
  return true;
}

bool FileProtoOps::cleanup_excess_version_dirs()
{
  unsigned vmax = 0;
  unsigned vcount = 0;

  std::tie (vmax, vcount) = get_max_ver_num_and_count();
  if (vcount <= 8) {
    return true;
  }

  unsigned svnum  = vmax - vcount + 1;
  unsigned to_del = vcount - 8;

  for (unsigned vern = svnum; vern < (svnum + to_del); ++vern) {

    std::ostringstream strm;
    strm << std::setfill('0') << std::setw(8) << std::hex << std::uppercase << vern;
    std::string ver_dir = schema_ver_dir_ + strm.str();

    //std::cout << "Removing excess version directory " << ver_dir << std::endl;
    fs_remove_directory(ver_dir);
  }

  return true;
}

bool FileProtoOps::cleanup_stale_version_dirs()
{
  unsigned vmax = 0;
  unsigned vcount = 0;

  std::tie (vmax, vcount) = get_max_ver_num_and_count();

  unsigned svnum  = vmax - vcount + 1;
  for (unsigned vern = svnum; vern < (svnum + vcount); ++vern) {

    std::ostringstream strm;
    strm << std::setfill('0') << std::setw(8) << std::hex << std::uppercase << vern;

    std::string ver_dir    = schema_ver_dir_ + strm.str();
    std::string stamp_file = ver_dir + "/stamp";

    if (!fs::exists(stamp_file) && (vern != vmax)) {
      //std::cout << "Removing stale version directory " << ver_dir << std::endl;
      fs_remove_directory(ver_dir);
    }
  }

  return true;
}

bool FileProtoOps::init_schema_dir()
{
  cleanup_tmp_files();
  cleanup_lock_files();
  return prune_schema_dir();
}

bool FileProtoOps::prune_schema_dir()
{
  cleanup_excess_version_dirs();
  cleanup_stale_version_dirs();
  return true;
}

bool FileProtoOps::remove_confd_ws(const char* prefix)
{
  for (fs::directory_iterator entry(rift_root_);
        entry != fs::directory_iterator(); ++entry)
  {
    if (!fs::is_directory(entry->path())) continue;

    auto dir_name = entry->path().filename().string();
    auto pos = dir_name.find(prefix);

    if (pos != 0) continue;

    if (!fs_remove_directory(entry->path().string())) {
      return false;
    }
  }
  return true;
}

bool FileProtoOps::remove_unique_confd_ws()
{
  // Remove the archived one as well
  return remove_confd_ws(CONFD_PWS_PREFIX) && remove_confd_ws(AR_CONFD_PWS_PREFIX);
}

bool FileProtoOps::remove_non_unique_confd_ws()
{
  return remove_confd_ws(CONFD_WS_PREFIX);
}

bool FileProtoOps::archive_confd_persist_ws()
{
  for (fs::directory_iterator entry(rift_root_);
        entry != fs::directory_iterator(); ++entry)
  {
    if (!fs::is_directory(entry->path())) continue;

    auto dir_name = entry->path().filename().string();
    auto pos = dir_name.find(CONFD_PWS_PREFIX);

    if (pos != 0) continue;
    // rename the persist directory
    auto mod_time = fs::last_write_time(entry->path());
    auto epoch = std::chrono::duration_cast<std::chrono::seconds> (
            std::chrono::system_clock::from_time_t(mod_time).time_since_epoch()).count();

    auto new_dir_name = rift_root_+ "/ar_" + dir_name + "_" + std::to_string(epoch);

    fs::rename(entry->path(), fs::path(new_dir_name));
    if (!fs::exists(new_dir_name)) {
      std::cerr << "Rename failed: " << new_dir_name << std::endl;
      return false;
    }
  }

  return true;
}

int main(int argc, char* argv[])
{
  if (argc < 2) {
    std::cerr << "Insufficient number of arguments provided" << std::endl;
    CmdParser(argv, argc).print_help();
    return EXIT_FAILURE;
  }

  CmdParser cmd(argv, argc);

  if (cmd.cmd_option_exists("-h") || cmd.cmd_option_exists("--help")) {
    cmd.print_help();
    return EXIT_SUCCESS;
  }

  FileProtoOps::init_cmd_map();
  FileProtoOps fops;

  bool created_lock = false;
  auto oper = cmd.get_next_operation();
  while (oper != nullptr) {
    std::string const operation(oper);

    if (fops.validate_cmd(oper)) {
      if ( !fops.execute_cmd(oper) ) {
        if (created_lock) {
          // if we have created the lock file, clean it up
          fops.execute_cmd("--lock-file-delete");
        }
        return EXIT_FAILURE;
      } else {
        // success
        if (operation == "--lock-file-create") {
          created_lock = true;
        }
      }
    } else {
      std::cerr << "ERROR: invalid option provided: " << oper << "\n";
      if (created_lock) {
        // if we have created the lock file, clean it up
        fops.execute_cmd("--lock-file-delete");
      }
      return EXIT_FAILURE;
    }

    oper = cmd.get_next_operation();
  }

  return EXIT_SUCCESS;
}
