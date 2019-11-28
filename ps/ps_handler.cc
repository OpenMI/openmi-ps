/*!
 *  Copyright (c) 2017 by Contributors
 *  \file ps_handler.cc
 *  \brief parameter server handler
 */
#include "ps_handler.h"

#include <iostream>
#include <string>
#include <boost/algorithm/string/join.hpp>
#include <gflags/gflags.h>
#include <google/protobuf/text_format.h>
#include <gperftools/malloc_extension.h>
#include "base/file_op.h"
#include "base/logging.h"
#include "base/protobuf_op.h"
#include "base/read_op.h" // MMap
#include "base/shell_util.h"
#include "base/thread_local.h"
#include "base/timer.h"
#include "base/write_op.h"
#include "openmi/idl/proto/communication.pb.h"
#include "openmi/idl/proto/model.pb.h"
using namespace openmi;

DEFINE_string(ckpt_basepath, "", "model checkpoint path");
DEFINE_string(ckpt_summarypath, "", "checkpoint meta path");

DEFINE_bool(online_ps, false, "start ps for online predictor serving");
DEFINE_string(ps_type, "offline", "ps serving type. offline | online | nearline");

inline size_t SizeSum(const std::vector<std::string>& str_list) {
  size_t size = 0;
  for (auto& s: str_list) {
    size += s.size();
  }
  return size;
} // SizeSum

namespace openmi {
namespace ps {

PsHandler::PsHandler(std::string& mode): mode_(mode) { 
  model_manager_ = std::make_shared<ModelManager>();
  if (FLAGS_online_ps) {
    FLAGS_ps_type = "online";
    if (FLAGS_ckpt_basepath != "") {
      RestoreModels();    // 如果模型ckpt路径存在，在线PS模式下 直接重新加载模型
    }
  }
}
  
PsHandler::~PsHandler() {}

int PsHandler::InitSession(PsSessionPtr session) {
  auto name = session->Name();
  auto model = model_manager_->GetModel(name);
  if (model == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ". name '" << name << "' not exist in model manager.";
    return -1;
  }
  session->Init(model);
  return 0;
}

PsSessionPtr PsHandler::GetSession(const std::string& name) {
  //static thread_local std::unordered_map<std::string, PsSessionPtr>* sessions = nullptr;  // TODO for linux
  static openmi::ThreadLocal<std::unordered_map<std::string, PsSessionPtr>*> sessions;
  if (sessions.Value() == nullptr) {
    sessions.Value() = new std::unordered_map<std::string, PsSessionPtr>();
  }
  auto& value = sessions.Value();
  PsSessionPtr session;
  auto it = value->find(name);
  if (it == value->end()) {
    session = std::make_shared<PsSession>(name);
    value->insert({name, session});
  } else {
    session = it->second;
  }
  if (InitSession(session) != 0) {
    LOG(INFO) << __FUNCTION__ << " init session failed.";
    return nullptr;
  }
  return session;
} // GetSession

void PsHandler::Pull(std::vector<std::string>& _return, 
                     const std::string& name, 
                     const std::vector<std::string>& reqs, 
                     const std::string& value_type, 
                     const std::string& req_id) {
  try {
    Timer time;
    auto session = GetSession(name);
    if (session == nullptr) {
      throw std::runtime_error("session is null. it maybe model not exists. name:" + name);
    }
    session->Pull(_return, reqs, value_type, req_id);
    LOG(INFO) << __FUNCTION__ << " value:" << value_type << ", name:" << name << ", time:" << time.Elapsed();
  } catch (std::exception& e) {
    LOG(ERROR) << __FUNCTION__ << " Exception: " << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " Unknown exception!!!";
    throw;
  }
  /*
  try {
    Timer t;
    if (reqs.size() != SHARD_NUM) {
      throw std::runtime_error("Shard num not match!!! Expect[" + std::to_string(SHARD_NUM) + "], Actual[" + std::to_string(reqs.size()) + "]");
    }
    pb::ValType vt;
    if (!ValType_Parse(val_type, &vt)) {
      throw std::runtime_error("Unknown val_type[" + val_type + "]. It must be one of [PARAMS, WEIGHTS, GRADS]");
    } else if (FLAGS_online_ps && vt != pb::WEIGHTS) {
      throw std::runtime_error("Can not pull val_type[" + val_type + "] from online serving ps. It must be 'WEIGHTS'!!!");
    } else if (!FLAGS_online_ps && (vt != pb::WEIGHTS && vt != pb::PARAMS)) {
      throw std::runtime_error("Can not pull val_type[" + val_type + "] from offline training ps. It must be one of [WEIGHTS, PARAMS]!!!");
    }
    auto session = GetSession(names);
    auto fid_cnt = session->Pull(reqs, vt, rsps);
    session->ClearModels();
    int proc_time = t.Elapsed();
    LOG(INFO) << "pull t " << proc_time << " fid_cnt[" << fid_cnt << "] req_size[" << SizeSum(reqs) << "] rsp_size[" << SizeSum(rsps) << "] " << session->Signature();
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception: " << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " unknown exception!!!";
    throw;
  }
  */
}

void PsHandler::Push(const std::string& name, const std::vector<std::string>& reqs, const std::string& value_type, const int32_t epoch, const std::string& req_id) {
  try {
    Timer time;
    auto session = GetSession(name);
    session->Push(reqs, value_type, req_id);
    LOG(INFO) << __FUNCTION__ << " value:" << value_type << ", name:" << name << ", time:" << time.Elapsed();
  } catch (std::exception& e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception " << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " unknown exception!!!";
    throw;
  }
  /*
  SetLogId(log_id);
  try {
    Timer t;
    if (reqs.size() != SHARD_NUM) {
      throw std::runtime_error("Shard num not match!!! Expect[" + std::to_string(SHARD_NUM) + "] Actual[" + std::to_string(reqs.size()) + "]");
    }
    pb::ValType vt;
    if (!ValType_Parse(val_type, &vt)) {
      throw std::runtime_error("Unknown val_type[" + val_type + "]. It must be one of [WEIGHTS, PARAMS, GRADS].");
    } else if (FLAGS_online_ps && vt != pb::WEIGHTS) {
      throw std::runtime_error("Can not push " + val_type + " to online serving ps!!! It muse be WEIGHTS!!!");
    } else if (!FLAGS_online_ps && (vt != pb::GRADS && vt != pb::PARAMS)) {
      throw std::runtime_error("Can not push " + val_type + " to offline serving ps!!! It must be one of [GRADS, PARAMS]!!!");
    }
    auto session = GetSession(names);
    auto fid_cnt = session->Push(reqs, vt, epoch);
    session->ClearModels();
    int proc_time = t.Elapsed();
    LOG(INFO) << "push t " << proc_time << " fid_cnt[" << fid_cnt << "] req_size[" << SizeSum(reqs) << "] " << session->Signature();
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception: " << e.what();
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " unknown exception!!!";
  }
  */
} // Push

void PsHandler::Create(std::string& _return, const std::string& graph_def, bool pb_binary) {
  try {
    Timer t;
    _return = model_manager_->Create(graph_def, pb_binary);
    LOG(WARN) << "create model. name '" << _return << "' time: " << t.Elapsed();
  } catch (std::exception& e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception: " << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " unknown exception";
    throw;
  }
}

void PsHandler::ModelDef(std::string& _return, const std::string& name, const bool pb_binary) {
  try {
    Timer t;
    model_manager_->GetModelDef(_return, name, pb_binary);
    LOG(WARN) << "get model def. name '" << name << "' time: " << t.Elapsed();
  } catch (std::exception& e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception: " << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " unknown exception";
    throw;
  }
}

void PsHandler::Drop(const std::string& name) {
  try {
    Timer t;
    model_manager_->Drop(name);
    MallocExtension::instance()->ReleaseFreeMemory();
    LOG(WARN) << __FUNCTION__ << " name:" << name << ", time:" << t.Elapsed();
  } catch (std::exception& e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception:" << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " unknown exception.";
    throw;
  }
} // Drop

// online ps: weight
// other ps: checkpoint
void PsHandler::Dump(const std::string& name, const std::string& path, const std::string& val_type, const std::string& format, const bool dump_zero) {
  LOG(INFO) << "PsHandler::Dump ...";
  try {
    Timer time;
    proto::ModelValueType value_type;
    if (!proto::ModelValueType_Parse(val_type, &value_type)) {
      throw std::runtime_error("Dump. Unkown value_type:" + val_type);
    }
    if (FLAGS_online_ps && value_type != proto::WEIGHT) {
      throw std::runtime_error("online ps can't dump value_type'" + 
        proto::ModelValueType_Name(value_type) + "'. It must be WEIGHT!");
    }
    if (!FLAGS_online_ps && (value_type != proto::WEIGHT && value_type != proto::CHECKPOINT)) {
      throw std::runtime_error("offline ps can't dump value_type'" + 
        proto::ModelValueType_Name(value_type) + "'. It must be one of [WEIGHT, CHECKPOINT]!");
    }
    auto session = GetSession(name);
    session->Dump(path, value_type, format, dump_zero);

    // 更新checkpoint.meta
    bool add_to_ckpt_meta = true;
    proto::CheckpointMeta ckpt_meta;
    std::string ckpt_meta_path = FLAGS_ckpt_basepath + "/checkpoint.meta";
    if (FileOp::access_file(ckpt_meta_path.c_str())) {
      if (ProtobufOp::LoadObjectFromPbFile<proto::CheckpointMeta>(ckpt_meta_path.c_str(), &ckpt_meta) != 0) {
        LOG(ERROR) << "parse checkpoint meta from file failed.";
        return;
      } 

      for (int i = 0; i < ckpt_meta.model_meta_size(); ++i) {
        if (name == ckpt_meta.model_meta(i).model_name()) {
          add_to_ckpt_meta = false;
          break;
        }
      }
    }

    if (add_to_ckpt_meta) {
      if (!FileOp::access_dir(FLAGS_ckpt_basepath.c_str())) {
        if (!FileOp::mk_dir(FLAGS_ckpt_basepath.c_str())) {
          LOG(ERROR) << "mkdir failed. path:" << FLAGS_ckpt_basepath;
          return;
        }
      }

      proto::DumpModelMeta* dump_meta = ckpt_meta.add_model_meta();
      dump_meta->set_model_name(name);
      dump_meta->set_path(path);
      dump_meta->set_value_type(proto::WEIGHT);
      
      Write write(ckpt_meta_path.c_str());
      std::string ckpt = ckpt_meta.DebugString();
      write.write(ckpt.c_str(), ckpt.size());
      write.close();
    }

    LOG(WARN) << __FUNCTION__ << " name:" << name << ", time:" << time.Elapsed();
  } catch (std::exception& e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception:" << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " unknown exception";
    throw;
  }
  /*
  try {
    Timer t;
    pb::ValType val_type;
    if (!ValType_Parse(_val_type, &val_type)) {
      throw std::runtime_error("Unknown val_type[" + _val_type + "]!!!");
    } else if (FLAGS_online_ps && val_type != pb::WEIGHTS) {
      throw std::runtime_error("Can not dump " + pb::ValType_Name(val_type) + " from online serving ps!!! It muse be WEIGHTS.");
    } else if (!FLAGS_online_ps && (val_type != pb::WEIGHTS && val_type != pb::PARAMS)) {
      throw std::runtime_error("Can not dump " + pb::ValType_Name(val_type) + " from offline serving ps!!! It muse be one of [WEIGHTS, PARAMS].");
    }
    pb::Format format;
    if (!Format_Parse(_format, &format)) {
      throw std::runtime_error("Unknown format[" + _format + "]!!!");
    }
    pb::DumpMeta meta;
    meta.set_path(path);
    meta.set_val_type(val_type);
    meta.set_format(format);
    meta.set_dump_zero(dump_zero);
    auto session = GetSession(names);
    session->Dump(meta);
    session->ClearModels();
    LOG(WARN) << "dump t " << t.Elapsed() << " " << val_type << " " << format << " path: " << path << " " << session->Signature();
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception: " << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " unknown exception";
    throw;
  }
  */
} // Dump

void PsHandler::DumpAll(const std::string& val_type, const std::string& format, const bool dump_zero) {
  LOG(INFO) << "PsHandler::DumpAll...";
  /*
  try {
    Timer t;
    long curtimestamp = t.CurTimeStamp();
    pb::CkptMeta data;
    std::ofstream ofs;
    ofs.open(FLAGS_ckpt_summarypath);
    if (!ofs) {
      throw std::runtime_error("Open ckpt summpary failed. file[" + FLAGS_ckpt_summarypath + "]!!!");
    }

    std::vector<thrift::ModelInfo> models;
    ListModels(models);

    std::unordered_map<std::string, std::vector<thrift::ModelInfo>> models_info_map;
    for (size_t i = 0; i < models.size(); ++i) {
      auto itr = models_info_map.find(models[i].group_name);
      if (itr == models_info_map.end()) {
        std::vector<thrift::ModelInfo> sub_models;
        models_info_map[models[i].group_name] = sub_models;
      }
      models_info_map[models[i].group_name].push_back(models[i]);
    }

    for (auto itr = models_info_map.begin(); itr != models_info_map.end(); ++itr) {
      std::vector<std::string> model_names;
      std::string group_name = itr->first;
      std::string dump_path = FLAGS_ckpt_basepath + itr->first;

      std::sort(itr->second.begin(), itr->second.end(), [](thrift::ModelInfo model_a, thrift::ModelInfo model_b) {
        return model_a.model_index < model_b.model_index;
      });

      for (size_t i = 0; i < itr->second.size(); ++i) {
        model_names.push_back(itr->second[i].model_name);
      }
      Dump(model_names, dump_path, val_type, format, dump_zero);
      data.add_group_names(group_name);
      data.add_ckpt_paths(dump_path);
    }

    data.set_timestamp(curtimestamp);
    ofs << data.DebugString() << std::endl;
    LOG(INFO) << "Dump all t " << t.Elapsed();
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception: " << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " unknown exception.";
    throw;
  }
  */
} // DumpAll

// load from local 
void PsHandler::Load(const std::string& name, const std::string& path) {
  try {
    Timer time;
    if (model_manager_->IsExist(name)) {
      LOG(WARN) << "model '" << name << "' has exists. it can't load from path:" << path;
      return;
    }

    auto graph_def_path = path + "/" + name + ".graph";
    proto::GraphDef gdef;
    if (ProtobufOp::LoadObjectFromPbFile<proto::GraphDef>(graph_def_path.c_str(), &gdef) != 0) {
      LOG(ERROR) << "load proto object from file failed. path:" << graph_def_path;
      return;
    }
    // TODO: dumped model should also be serialized binary string and md5
    auto model_name = model_manager_->Create(gdef.DebugString(), false);
    if (name != model_name) {
      LOG(WARN) << "name != model_name in graph";
    }

    auto session = GetSession(name);
    session->Load(path, proto::WEIGHT, "format");

    LOG(WARN) << "load time:" << time.Elapsed() << " " << name << " path: " << path;
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception: " << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " unknown exception.";
    throw;
  }
} // Load 

void PsHandler::RestoreModels() {
  Timer time;
  proto::CheckpointMeta ckpt_meta;
  try {
    if (!FileOp::access_dir(FLAGS_ckpt_basepath.c_str())) {
      LOG(WARN) << "checkpoint basepath can not access it. path:" << FLAGS_ckpt_basepath;
      return;
    }
    std::string ckpt_meta_path = FLAGS_ckpt_basepath + "/checkpoint.meta";
    if (ProtobufOp::LoadObjectFromPbFile<proto::CheckpointMeta>(ckpt_meta_path.c_str(), &ckpt_meta) != 0) {
      LOG(ERROR) << "load checkpoint meta from file failed. path:" << ckpt_meta_path;
      return;
    }
    for (int i = 0; i < ckpt_meta.model_meta_size(); ++i) {
      auto model_meta = ckpt_meta.model_meta(i);
      LOG(INFO) << "load model info:\n" << model_meta.DebugString();
      Load(model_meta.model_name(), model_meta.path());
      LOG(INFO) << "load model done. model name:" << model_meta.model_name();
    }
    LOG(INFO) << __FUNCTION__ << ". time:" << time.Elapsed();
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception: " << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " unknown exception.";
    throw;
  }
} // RestoreModels 

void PsHandler::SetStreamingUpdate(const std::vector<std::string>& names, const bool streaming_update) {
  LOG(INFO) << "PsHandler::SetStreamingUpdate ...";
  /*
  try {
    Timer t;
    auto session = GetSession(names);
    session->SetStreamingUpdate(streaming_update);
    session->ClearModels();
    LOG(WARN) << "set streaming t " << t.Elapsed() << " " << session->Signature();
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception: " << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " unknown exception,";
    throw;
  }
  */
} // SetStreamingUpdate

void PsHandler::GetUpdated(std::vector<std::string>& rsps, const std::vector<std::string>& names) {  
  LOG(INFO) << "PsHandler::GetUpdated ...";
  /*
  try {
    Timer t;
    auto session = GetSession(names);
    session->GetUpdated(rsps);
    session->ClearModels();
    LOG(INFO) << "get_updated t " << t.Elapsed() << " " << session->Signature();
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception: " << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " unknown exception";
    throw;
  }
  */
} // GetUpdated

void PsHandler::Stat(std::vector<std::string>& stats, const std::vector<std::string>& names) {
  LOG(INFO) << "PsHandler::Stat ...";
  /*
  try {
    Timer t;
    auto session = GetSession(names);
    session->Stat(stats);
    session->ClearModels();
    LOG(INFO) << "get_updated t " << t.Elapsed() << " " << session->Signature();
  } catch (std::exception& e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception: " << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " unknown exception";
    throw;
  }
  */
} // Stat

void PsHandler::ListModels(std::vector<std::string>& models) {
  try {
    Timer time;
    model_manager_->ListModels(models);
    LOG(WARN) << __FUNCTION__ << " time: " << time.Elapsed();
  } catch (std::exception& e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception: " << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " unknown exception";
    throw;
  }
} // ListModels

void PsHandler::Move(const std::string& from_name, const std::string& to_name, const std::string& backup_name) {
  try {
    Timer time;
    model_manager_->Move(from_name, to_name, backup_name);
    LOG(WARN) << __FUNCTION__ <<  " from[" << from_name << "], to[" 
              << to_name << "], backup[" << backup_name << "], time:" << time.Elapsed(); 
  } catch (std::exception& e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception: " << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " Unknown exception";
    throw;
  }
} // Move

void PsHandler::ServingType(std::string& serving_type) {
  try {
    Timer t;
    if (FLAGS_online_ps) {
      serving_type = "online";
    } else {
      serving_type = FLAGS_ps_type;
    }
    LOG(WARN) << "serving_type t" << t.Elapsed(); 
  } catch (std::exception& e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception: " << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " unknown exception.";
    throw;
  }
} // ServingType

void PsHandler::ExecShell(std::vector<std::string>& outputs, const std::vector<std::string>& cmds) {
  try {
    Timer t;
    if (!FLAGS_online_ps) {
      for (auto& cmd: cmds) {
        int ret = openmi::Exec(cmd, outputs);
        if (ret != 0) break;
      }
    }
    LOG(WARN) << "exec_shell t " << t.Elapsed();
  } catch (std::exception& e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception: " << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " unknown exception.";
    throw;
  }
} // ExecShell

int32_t PsHandler::ShardNum() {
  return 8;
  //return SHARD_NUM;
}

void PsHandler::Mode(std::string& rsp) {
  rsp = mode_;
}

}} // namespace
