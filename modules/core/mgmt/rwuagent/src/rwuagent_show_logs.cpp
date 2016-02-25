
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
* @file rwuagent_show_sys_info.cpp
* @date 2015/12/28
* Management agent show agent logs command
*/

#include <string>
#include <vector>
#include <fstream>
#include <ios>
#include "rwuagent.hpp"
#include "rw-mgmtagt.pb-c.h"

StartStatus output_to_console(Instance*, SbReqRpc*);
StartStatus output_to_file(Instance*, SbReqRpc*, const char*);

StartStatus read_logs_and_send(Instance* instance,
                  SbReqRpc* rpc,
                  const RWPB_T_MSG(RwMgmtagt_input_ShowAgentLogs)* req)
{
  RW_ASSERT (req);
  RW_MA_INST_LOG(instance, InstanceInfo, "Reading management system logs.");

  if (req->has_console) {
    return output_to_console(instance, rpc);
  } else {
    return output_to_file(instance, rpc, req->file);
  }
}

std::string get_log_records(Instance* instance, 
    const std::string& file)
{
  RW_MA_INST_LOG (instance, InstanceDebug, "Get log records from log file");
  std::string records;
  std::ifstream in(file, std::ios::in | std::ios::binary);
  if (in) {
    std::ostringstream contents;
    contents << in.rdbuf();
    records = std::move(contents.str());
    in.close();
  } else {
    std::string log("Log file not found: ");
    log += file;
    RW_MA_INST_LOG (instance, InstanceError, log.c_str());
  }
  return records;
}

StartStatus output_to_console(Instance* instance,
                              SbReqRpc* rpc)
{
  RW_MA_INST_LOG (instance, InstanceDebug, "Output logs records to console");

  auto log_files = instance->startup_hndl()->get_log_files();

  RWPB_M_MSG_DECL_INIT(RwMgmtagt_AgentLogsOutput, output);
  output.n_result = log_files.size();
  output.result = (RWPB_T_MSG(RwMgmtagt_AgentLogsOutput_Result) **)
     RW_MALLOC (sizeof (RWPB_T_MSG(RwMgmtagt_AgentLogsOutput_Result)*) * output.n_result);

  std::vector<RWPB_T_MSG(RwMgmtagt_AgentLogsOutput_Result)>
    results (output.n_result);
  std::vector<std::string> records(output.n_result);
  size_t idx = 0;

  for (auto& file: log_files) {
    RWPB_M_MSG_DECL_INIT(RwMgmtagt_AgentLogsOutput_Result, res);
    records[idx] = get_log_records(instance, file);
    res.log_records = &records[idx][0];
    res.log_name = &log_files[idx][0];
    results[idx] = res;

    output.result[idx] = &results[idx];
    idx++;
  }

  // Send the response
  rpc->internal_done(
      &RWPB_G_PATHSPEC_VALUE(RwMgmtagt_AgentLogsOutput)->rw_keyspec_path_t, &output.base);

  RW_FREE (output.result);

  return StartStatus::Done;
}

StartStatus output_to_file(Instance* instance,
                           SbReqRpc* rpc,
                           const char* file_name)
{
  RW_MA_INST_LOG (instance, InstanceDebug, "Output logs records to file");

  const char* rift_root = getenv("RIFT_INSTALL");
  if (nullptr == rift_root) {
    rift_root = "/";
  }

  std::string ofile = (std::string(rift_root) + "/") + file_name;
  std::ofstream out(ofile);

  auto log_files = instance->startup_hndl()->get_log_files();
  for (auto& file: log_files) {
    out << "------" << file << "-----------\n";
    out << get_log_records(instance, file);
    out << "-------------------------------\n";
  }
  out.close();

  if (!out) {
    // error
    NetconfErrorList nc_errors;
    NetconfError& err = nc_errors.add_error();
    std::string log("Error while writing to file: ");
    log += ofile;
    err.set_error_message(log.c_str());
    rpc->internal_done(&nc_errors);
  } else {
    RWPB_M_MSG_DECL_INIT(RwMgmtagt_AgentLogsOutput, output);
    output.n_result = 1;
    output.result = (RWPB_T_MSG(RwMgmtagt_AgentLogsOutput_Result) **)
       RW_MALLOC (sizeof (RWPB_T_MSG(RwMgmtagt_AgentLogsOutput_Result)*) * output.n_result);

    RWPB_M_MSG_DECL_INIT(RwMgmtagt_AgentLogsOutput_Result, res);
    output.result[0] = &res;

    res.log_name = (char*)"All files";
    std::string rec("Log records saved to file: ");
    rec += ofile;
    res.log_records = &rec[0];

    rpc->internal_done(
         &RWPB_G_PATHSPEC_VALUE(RwMgmtagt_AgentLogsOutput)->rw_keyspec_path_t, &output.base);
    RW_FREE (output.result);
  }

  return StartStatus::Done;
}

