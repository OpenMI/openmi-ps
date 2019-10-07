/*!
 *  Copyright (c) 2017 by Contributors
 *  \file ps_handler.h
 *  \brief parameter server handler
 */
#ifndef OPENMI_PS_PS_HANDLER_H_
#define OPENMI_PS_PS_HANDLER_H_

#include <memory>
#include <string>

#include "gen-thrift/gen-cpp/Ps.h" 
#include "gen-thrift/gen-cpp/ps_types.h"

//#include "model_manager.h"
//#include "session.h"

using namespace openmi::thrift;

namespace openmi {
namespace ps {

class PsHandler: virtual public PsIf {
public:
  PsHandler(std::string& mode);
  
  ~PsHandler();

  void Pull(std::vector<std::string>& rsps, const std::vector<std::string>& names, const std::vector<std::string>& reqs, const std::string& val_type, const int64_t log_id) override;

  void Push(const std::vector<std::string>& names, const std::vector<std::string>& reqs, const std::string& val_type, const int32_t epoch, const int64_t log_id) override; 

  void Create(std::vector<std::string>& names, const std::string& name, const std::vector<std::string>& pb_defs, bool pb_binary) override;

  void Drop(const std::string& name) override;

  void Dump(const std::vector<std::string>& names, const std::string& path, const std::string& val_type, const std::string& format, const bool dump_zero) override;

  void DumpAll(const std::string& val_type, const std::string& format, const bool dump_zero) override;

  void Load(const std::string& name, const std::string& path) override;

  void RestoreModels() override;

  void SetStreamingUpdate(const std::vector<std::string>& names, const bool streaming_udpate) override;

  void GetUpdated(std::vector<std::string>& rsps, const std::vector<std::string>& names) override;

  void Stat(std::vector<std::string>& stats, const std::vector<std::string>& names) override;

  void ListModels(std::vector<ModelInfo>& models) override;

  void Move(const std::string& from_name, const std::string& to_name, const std::string& backup_name) override;

  void ModelDef(std::string& model_def, const std::string& name, const bool pb_binary=false) override;

  void ServingType(std::string& serving_type) override;

  void ExecShell(std::vector<std::string>& outputs, const std::vector<std::string>& cmds) override;

  int32_t ShardNum() override;

  void Mode(std::string& rsp) override;

private:
  //void InitSessionModels(std::shared_ptr<PsSession> session);

  //PsSessionPtr GetSession(const std::vector<std::string>& names);

private:
  std::string mode_;
  //std::shared_ptr<ModelManager> model_manager_;  // model manager
};

}} // namespace
#endif // OPENMI_PS_PS_HANDLER_H_
