/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 * Creation Date: 12/11/2015
 * 
 */

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <set>
#include <stdlib.h>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>

#include "rwdts.h"
#include "rwdts_api_gi.h"
#include "rwdts_int.h"
#include "rwdts_member_api.h"
#include "rwdts_query_api.h"
#include "rwmemlog.h"
#include "rwyangutil.h"

#include "rwdynschema_load_schema.h"
#include "rwdynschema.h"

namespace fs = boost::filesystem;

namespace {

std::string execute_command_and_get_result(std::string const cmd)
{
  std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
  RW_ASSERT(pipe);

  size_t const buffer_size = 128;
  char buffer[buffer_size];
  std::string result = "";
  while (!feof(pipe.get())) {
    if (fgets(buffer, buffer_size, pipe.get()) != NULL)
      result += buffer;
  }

  return result;
}

std::vector<std::string> collect_yang_modules(fs::path const & directory,
    rwmemlog_buffer_t* memlog_buf)
{
  RW_ASSERT_MESSAGE(fs::is_directory(directory),
                    "yang modules directory doesn't exist: %s\n",
                    directory.string().c_str());

  std::vector<std::string> collection;

  std::set<std::string> const exclusion_list = {"yuma-ncx",
                                                "yangdump",
                                                "yuma-types",
                                                "yuma-app-common",
                                                "rw-composite"};
    
  fs::directory_iterator end_iter; // default construction represents past-the-end
  for (fs::directory_iterator current_file(directory);
       current_file != end_iter;
       ++current_file) {
  
    if (!fs::is_regular_file(current_file->path())) {
      RWMEMLOG(&memlog_buf, RWMEMLOG_MEM2, "err file doesnt exist",
          RWMEMLOG_ARG_STRNCPY(MEMLOG_MAX_PATH_SIZE, 
                        current_file->path().string().c_str()));
      continue;
    }
    std::string const current_module_name = current_file->path().stem().string();

    if (exclusion_list.count(current_module_name)) {
      continue;
    }

    collection.emplace_back(current_module_name);
  }

  return collection;
}

std::vector<std::string> collect_shared_objects(
    std::vector<std::string> const & yang_modules,
    rwmemlog_buffer_t* memlog_buf)
{
  std::vector<std::string> shared_objects;

  std::string const rift_install_dir = getenv("RIFT_INSTALL");
  std::string const prefix_directory = rift_install_dir + "/usr";

  if (!(fs::exists(prefix_directory) && fs::is_directory(prefix_directory))) {
    RWMEMLOG(&memlog_buf, RWMEMLOG_MEM2, "err prefix dir not found ",
        RWMEMLOG_ARG_STRNCPY(MEMLOG_MAX_PATH_SIZE, prefix_directory.c_str()));
    return std::vector<std::string>();
  }

  for (std::string const & current_module : yang_modules) {
    // RIFT-10910 workaround: --define-variable
    std::string const command = "pkg-config --define-variable=prefix=" + prefix_directory
                                + " --libs " + current_module;
    std::string const result = execute_command_and_get_result(command);
    if (!result.length()) {
      RWMEMLOG(&memlog_buf, RWMEMLOG_MEM2, "pkg-config command failed");
      continue;
    }

    // .so path and lib name are space-separated
    size_t const split_index = result.find(' ') ;
    if (split_index == std::string::npos) {
      continue;
    }

    // remove -L prefix
    std::string const lib_path = result.substr(2, split_index - 2);

    // remove -l prefix and trailing whitespace
    std::string lib_name = result.substr(3 + split_index);
    lib_name = lib_name.substr(0, lib_name.find_first_of(" \n"));

    std::string const so_path = lib_path + "/lib" + lib_name + ".so";
    if (!fs::is_regular_file(so_path)) {
      RWMEMLOG(&memlog_buf, RWMEMLOG_MEM2, "Err SO path not found or regular ",
          RWMEMLOG_ARG_STRNCPY(MEMLOG_MAX_PATH_SIZE, so_path.c_str()));
      continue;
    }

    shared_objects.emplace_back(so_path);
  }

  return shared_objects;
}

void populate_rwdynschema_instance(rwdynschema_dynamic_schema_registration_t * reg,
                                   std::vector<std::string> const & yang_modules,
                                   std::vector<std::string> const & so_filenames)
{
  reg->batch_size = 0;
  size_t const module_count = yang_modules.size();
  for (size_t i = 0; i < module_count; ++i) {
    rwdynschema_add_module_to_batch(reg,
                                    yang_modules[i].c_str(),
                                    nullptr, //fxs_filename
                                    so_filenames[i].c_str(),
                                    nullptr); //yang_filename
  }

}

}

void rwdynschema_load_all_schema(void * context)
{

  rwdynschema_dynamic_schema_registration_t * reg =
      reinterpret_cast<rwdynschema_dynamic_schema_registration_t *>(context);

  RW_ASSERT(reg);

  std::cout << reg->app_name << " try create schema directory" << std::endl;
  bool const created_schema_directory = rw_create_runtime_schema_dir();

  while (!created_schema_directory) {
    // try again
    rwsched_dispatch_async_f(reg->dts_handle->tasklet,
                             rwsched_dispatch_get_main_queue(reg->dts_handle->sched),
                             context,
                             rwdynschema_load_all_schema);
    return;
  }

  // make sure the lates directory exists
  std::string const rift_install_dir = getenv("RIFT_INSTALL");
  fs::path const latest_version_directory = rift_install_dir + "/" + LATEST_VER_DIR;
  RW_ASSERT_MESSAGE(fs::exists(latest_version_directory)
                    && fs::is_directory(latest_version_directory),
                    "schema directory doesn't exist: %s\n",
                    latest_version_directory.string().c_str());

  // collect yang module names
  fs::path const yang_directory = latest_version_directory / "yang";
  std::vector<std::string> yang_modules = collect_yang_modules(
      yang_directory, reg->memlog_buffer);
  
  // collect so filenames
  std::vector<std::string> so_filenames = collect_shared_objects(
      yang_modules, reg->memlog_buffer);

  populate_rwdynschema_instance(reg, yang_modules, so_filenames);

  // set YUMA_MODPATH to something reasonable
  std::string const rift_install = getenv("RIFT_INSTALL");
  std::string const yuma_modpath = rift_install + "/usr/data/yang"
                                   + ":" + latest_version_directory.string();
  size_t const overwrite_existing = 1;
  setenv("YUMA_MODPATH", yuma_modpath.c_str(), overwrite_existing);

  // push data through app callback
  RWMEMLOG_TIME_CODE((
      reg->app_sub_cb(reg->app_instance,
                      reg->batch_size,
                      reg->module_names,
                      nullptr,
                      reg->so_filenames,
                      nullptr)
                      ),
      &reg->memlog_buffer, RWMEMLOG_MEM2, "app_callback");

}


