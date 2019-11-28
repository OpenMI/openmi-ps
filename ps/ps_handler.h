/*!
 *  Copyright (c) 2017 by Contributors
 *  \file ps_handler.h
 *  \brief parameter server handler
 */
#ifndef OPENMI_PS_PS_HANDLER_H_
#define OPENMI_PS_PS_HANDLER_H_

#include <memory>
#include <string>

#include "openmi/gen-cpp/Ps.h" 
#include "openmi/gen-cpp/ps_types.h"

#include "model_manager.h"
#include "ps/ps_session.h"

using namespace openmi::thrift;

namespace openmi {
namespace ps {

class PsHandler: virtual public PsIf {
public:
  PsHandler(std::string& mode);
  
  ~PsHandler();

  void Pull(std::vector<std::string>& _return, 
            const std::string& name, 
            const std::vector<std::string>& reqs, 
            const std::string& value_type, 
            const std::string& req_id) override;

  void Push(const std::string& name, 
            const std::vector<std::string>& reqs, 
            const std::string& value_type, 
            const int32_t epoch, 
            const std::string& req_id) override;
  
  void Create(std::string& _return, 
              const std::string& graph_def, 
              const bool pb_binary=true) override;
  
  void ModelDef(std::string& _return, 
                const std::string& name, 
                const bool pb_binary=true) override;

  // dump online
  void Dump(const std::string& name, 
            const std::string& path, 
            const std::string& val_type, 
            const std::string& format, 
            const bool dump_zero) override;
  
  // void DumpToHdfs();

  void DumpAll(const std::string& val_type, 
               const std::string& format, 
               const bool dump_zero) override;

  void Load(const std::string& name, 
            const std::string& path) override;
  
  void Drop(const std::string& name) override;

  void Move(const std::string& from_name, 
            const std::string& to_name, 
            const std::string& backup_name) override;

  void RestoreModels() override;

  void SetStreamingUpdate(const std::vector<std::string>& names, const bool streaming_udpate) override;

  void GetUpdated(std::vector<std::string>& rsps, const std::vector<std::string>& names) override;

  void Stat(std::vector<std::string>& stats, const std::vector<std::string>& names) override;

  void ListModels(std::vector<std::string>& models) override;

  void ServingType(std::string& serving_type) override;

  void ExecShell(std::vector<std::string>& outputs, const std::vector<std::string>& cmds) override;

  int32_t ShardNum() override;

  void Mode(std::string& rsp) override;

private:
  PsSessionPtr GetSession(const std::string& name);
  int InitSession(PsSessionPtr session);

private:
  std::string mode_;
  ModelManagerPtr model_manager_;  // model manager
};

}} // namespace
#endif // OPENMI_PS_PS_HANDLER_H_
