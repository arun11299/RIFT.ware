
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */



/**
 * @file rwcli_schema.hpp
 * @author Balaji Rajappa (balaji.rajappa@riftio.com)
 * @date 10/14/2015
 * @brief Schema Manager for RW.CLI
 *
 * SchemaManager maintains the schemas loaded by the RW.CLI. A SchemaUpdater
 * within a SchemaManager monitors for any dynamic schema additions. If there a
 * new schema added the SchemaUpdater load the new schema into the SchemaManger
 * and the same is avaiable for the RW.CLI parsing and completion. 
 */

#ifndef __RWCLI_SCHEMA_HPP__
#define __RWCLI_SCHEMA_HPP__

#if __cplusplus < 201103L
#error "Requires C++11"
#endif

#ifdef RW_DOXYGEN_PARSING
#define __cplusplus 201103
#endif

#include <string>
#include <chrono>
#include <memory>
#include <list>
#include <iostream>
#include <unordered_map>
#include <utility>
#include "rwlib.h"
#include "yangncx.hpp"

// ATTN: Do we need a new namespace rw_cli?
namespace rw_yang {

// Forward declarations
class SchemaManager;

// Global typedefs
typedef std::chrono::time_point<std::chrono::system_clock> timestamp_t;

/**
 * Stores the loaded schema name and the timestamp.
 */
class SchemaInfo
{
public:
  enum SchemaState {
    SCHEMA_STATE_INIT,
    SCHEMA_STATE_LOADED,
  };

  /**
   * Constructor for SchemaInfo
   * @param[in] name  Name of the schema
   * @param[in] state State of the schema 
   * @param[in] ts    Time at which the schema was loaded
   */
  SchemaInfo(const std::string& name, SchemaState state, timestamp_t ts);

  /**
   * Default Descructor
   */
  ~SchemaInfo() = default;

  typedef std::unique_ptr<SchemaInfo> ptr_t;

 public:
  std::string name_;                      ///< Name of the schema
  SchemaState state_ = SCHEMA_STATE_INIT; ///< state of the schema
  timestamp_t load_timestamp_;            ///< schema load time stamp
};

/**
 * Abstract base class from which a concrete implementaion of SchemaUpdater
 * derives.
 */
class SchemaUpdater
{
public:

  SchemaUpdater(SchemaManager* mgr);
  virtual ~SchemaUpdater() {}

  typedef std::unique_ptr<SchemaUpdater> ptr_t;

public:

  /**
   * Returns the underlying handle that is used for monitoring the Schema
   * changes.
   */
  virtual int get_handle() = 0;

  /**
   * Checks the underlying handle for schema changes and if there is a schema
   * change the schema is loaded.
   */
  virtual int check_for_updates() = 0;

public:
  /**
   * Back referene to its owner
   */
  SchemaManager* mgr_ = nullptr;
};

/**
 * SchemaManager loads and maintains the schemas. Implements an SchemaUpdater
 * which monitors for dynamic schema change and if found, loads the schema.
 * SchemaManager also holds the YangModel (a collection of all the loaded
 * YangModules). 
 */
class SchemaManager
{
public:
 
  /**
   * Constructor for SchemaManager.
   *
   * Creates a SchemaUpater which monitors for schema change
   */ 
  SchemaManager();

  /**
   * Specialized constructor used for setting a custom schema directory.
   */
  SchemaManager(const std::string& schema_path);

  /**
   * Default destructor
   */
  ~SchemaManager() = default;

  typedef std::unordered_map<std::string, SchemaInfo> schema_map_t;

public:

  /**
   * Load all the yang modules present in the latest
   * valid schema version directory.
   */
  bool load_all_schemas();

  /**
   * Get the latest version cli manifest directory.
   */
  std::string get_cli_manifest_dir();

  /**
   * Load the schema into the YangModel.
   *
   * This will make the schema available of the RW.CLI.
   */
  rw_status_t load_schema(const std::string& schema_name);

  /**
   * Prints the schemas loaded into the provided output stream.
   */
  void show_schemas(std::ostream& os);

public:
  const char* CONFIG_ROOT_MODEL = "config_root";

  /**
   * Stores the loaded schemas
   */
  schema_map_t          schema_table_;

  /**
   * A schema updater, that monitors for schema change
   */
  SchemaUpdater::ptr_t  updater_;

  /**
   * Collection of all the loaded YangModules, which will be used by the CLI for
   * parsing and completion.
   */
  YangModel::ptr_t      model_;

  /**
   * Schema directory which can be used for monitoring
   */
  std::string           schema_path_;

  /**
   * The rift_install path
   */
  std::string           rift_install_;

};

/**
 * Schema File information. 
 *
 * This has the information about the filename which got changed and if the file
 * was created, modified or deleted. Used in a InotifySchemaUpdater.
 *
 * @note Currently only new files are handled.
 * @see_also InotifySchemaUpdater
 */
class SchemaFileInfo
{
public:
  enum SchemaFileState {
    SCHEMA_FILE_NONE,
    SCHEMA_FILE_CREATED,
    SCHEMA_FILE_MODIFIED,
    SCHEMA_FILE_DELETED
  };

  /**
   * Constructor for SchemaFileInfo
   * @param[in] filename  Filename that was changed.
   * @param[in] state     Type of change (create/modify/delete)
   */
  SchemaFileInfo(const std::string& filename, SchemaFileState state);

  typedef std::unique_ptr<SchemaFileInfo> ptr_t;

public:
  std::string filename_;  ///< Name of the file
  SchemaFileState state_ = SCHEMA_FILE_NONE;  ///< Type of file change
};

/**
 * Maintains a list of yang files that were changed in the schema-yang
 * directory. 
 */
class SchemaChangeset
{
public:
  SchemaChangeset() = default;
  ~SchemaChangeset() = default;

  typedef std::list<SchemaFileInfo> schema_list_t;

public:
  /**
   * Adds the changed files
   * @param[in] filename  Filename that was changed
   * @param[in] inotify_mask  inotify mask that has the information on the type
   *                          of file change
   */
  void add_change(const std::string& filename, uint32_t inotify_mask);

  /**
   * Clears the changeset.
   */
  void clear() {
    schema_files_.clear();
  }

public:
  schema_list_t schema_files_; ///< List of changed files
};

/**
 * InotifySchemaUpdater monitors for a given schema directory and load any new
 * schema found in that directory.
 *
 * File Update Protocol:
 * The base schema directory is /var/rift/schema. All Yang files goes into the
 * SCHEMA_BASE/yang. There is a SCHEMA_BASE/version directory which maintains
 * the version changes. 
 *  - When a schema change is detected, the files are copied into yang directory
 *  - A new version directory is created and yang files are linked in the
 *    new version directory
 *  - A stamp file is created in the new version once the confd upgrade is
 *    completed  
 *  - A new softlink confd_yang is created in the version directory
 *
 * The implementaion monitors for changes in the yang directory. All the added
 * new files are maintained in a SchemaChangeSet. Once the softlink confd_yang
 * is recreated, the files in the SchemaChangeSet is loaded.
 */
class InotifySchemaUpdater: public SchemaUpdater
{
public:
  /**
   * Constructor for the Inotify based SchemaUdpater
   */
  InotifySchemaUpdater(SchemaManager* mgr);

  /**
   * Destructor
   */
  ~InotifySchemaUpdater() override;

public:

  /**
   * Returns the underlying inotify file descriptor.
   *
   * The fd returned can be used by an event loop for monitoring events.
   */
  virtual int get_handle() override;

  /**
   * Checks for updates and loads new schema.
   *
   * This method will be invoked when an event loops find that there is an event
   * in the inotify fd.
   */
  virtual int check_for_updates() override;

  /**
   * Loads the modules that are present in the ChangeSet.
   */
  void load_modules();

public:

  int inotify_fd_ = -1; ///< Inotify File descriptor

  int yang_dir_watchfd_ = -1; ///< Watch descriptor for yang directory
  int ver_dir_watchfd_ = -1;  ///< Watch descriptor for version directory

  SchemaChangeset changeset_; ///< List of changed files
};


} //namespace rw_yang

#endif // __RWCLI_SCHEMA_HPP__
