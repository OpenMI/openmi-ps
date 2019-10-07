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
#include "base/logging.h"
#include "base//timer.h"
#include "base/read_op.h" // MMap
#include "base/shell_util.h"
using namespace openmi;

DEFINE_string(ckpt_basepath, "", "model checkpoint path");
DEFINE_string(ckpt_summarypath, "", "ckpt meta path");

DEFINE_bool(online_serving, false, "start ps for online serving");
DEFINE_string(serving_type, "offline", "ps serving type. offline | online | nearline");

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
  //model_manager_ = std::make_shared<ModelManager>();
  if (FLAGS_online_serving) {
    FLAGS_serving_type = "online";
    if (FLAGS_ckpt_summarypath != "") {
      RestoreModels();    // 如果模型ckpt路径存在，在线PS模式下 直接重新加载模型
    }
  }
}
  
PsHandler::~PsHandler() {}
/*
void PsHandler::InitSessionModels(std::shared_ptr<PsSession> session) {
  session->ClearModels();
  ModelGroupPtr model_group;
  std::vector<size_t> model_indexes;
  std::vector<size_t> data_indexes;
  auto& names = session->ModelNames();
  for (size_t i = 0; i < names.size(); ++i) {
    thrift::ModelInfo model_info;
    auto next_model_group = model_manager_->GetModel(model_info, names[i]);
    if (model_group && next_model_group != model_group) {
      session->AddModelGroup(model_group, model_indexes, data_indexes);
      model_indexes.clear();
      data_indexes.clear();
    }
    model_group = next_model_group;
    model_indexes.push_back(model_info.model_index);
    data_indexes.push_back(i);
  }
  session->AddModelGroup(model_group, model_indexes, data_indexes);
}

PsSessionPtr PsHandler::GetSession(const std::vector<std::string>& names) {
  //static thread_local std::unordered_map<std::string, PsSessionPtr>* sessions = nullptr;  // TODO for linux
  static std::unordered_map<std::string, PsSessionPtr>* sessions = nullptr;
  if (!sessions) {
    sessions = new std::unordered_map<std::string, PsSessionPtr>();
  }
  std::string sign = boost::join(names, ",");
  PsSessionPtr session;
  auto it = sessions->find(sign);
  if (it == sessions->end()) {
    session = std::make_shared<PsSession>(names);
    (*sessions)[sign] = session;
  } else {
    session = it->second;
  }
  InitSessionModels(session);
  return session;
} // GetSession
*/

void PsHandler::Pull(std::vector<std::string>& rsps, const std::vector<std::string>& names, const std::vector<std::string>& reqs, const std::string& val_type, const int64_t log_id) {
  /*
  SetLogId(log_id);
  try {
    Timer t;
    if (reqs.size() != SHARD_NUM) {
      throw std::runtime_error("Shard num not match!!! Expect[" + std::to_string(SHARD_NUM) + "], Actual[" + std::to_string(reqs.size()) + "]");
    }
    pb::ValType vt;
    if (!ValType_Parse(val_type, &vt)) {
      throw std::runtime_error("Unknown val_type[" + val_type + "]. It must be one of [PARAMS, WEIGHTS, GRADS]");
    } else if (FLAGS_online_serving && vt != pb::WEIGHTS) {
      throw std::runtime_error("Can not pull val_type[" + val_type + "] from online serving ps. It must be 'WEIGHTS'!!!");
    } else if (!FLAGS_online_serving && (vt != pb::WEIGHTS && vt != pb::PARAMS)) {
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

void PsHandler::Push(const std::vector<std::string>& names, const std::vector<std::string>& reqs, const std::string& val_type, const int32_t epoch, const int64_t log_id) {
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
    } else if (FLAGS_online_serving && vt != pb::WEIGHTS) {
      throw std::runtime_error("Can not push " + val_type + " to online serving ps!!! It muse be WEIGHTS!!!");
    } else if (!FLAGS_online_serving && (vt != pb::GRADS && vt != pb::PARAMS)) {
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

void PsHandler::Create(std::vector<std::string>& names, const std::string& name, const std::vector<std::string>& pb_defs, bool pb_binary) {
  /*
  try {
    Timer t;
    names = model_manager_->Create(name, pb_defs, pb_binary);
    LOG(WARNING) << "create t " << t.Elapsed() << " group[" << name << "] models[" << boost::join(names, ",") << "]";
  } catch (std::exception& e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception: " << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " unknown exception";
    throw;
  }
  */
} // Create

void PsHandler::Drop(const std::string& name) {
  /*
  LOG(INFO) << "PsHandler::Drop ...";
  try {
    Timer t;
    model_manager_->Drop(name);
    MallocExtension::instance()->ReleaseFreeMemory();
    LOG(WARNING) << "drop t=" << t.Elapsed() << " " << name;
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception: " << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " unknown exception";
    throw;
  }
  */
} // Drop

void PsHandler::Dump(const std::vector<std::string>& names, const std::string& path, const std::string& _val_type, const std::string& _format, const bool dump_zero) {
  LOG(INFO) << "PsHandler::Dump ...";
  /*
  try {
    Timer t;
    pb::ValType val_type;
    if (!ValType_Parse(_val_type, &val_type)) {
      throw std::runtime_error("Unknown val_type[" + _val_type + "]!!!");
    } else if (FLAGS_online_serving && val_type != pb::WEIGHTS) {
      throw std::runtime_error("Can not dump " + pb::ValType_Name(val_type) + " from online serving ps!!! It muse be WEIGHTS.");
    } else if (!FLAGS_online_serving && (val_type != pb::WEIGHTS && val_type != pb::PARAMS)) {
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
    LOG(WARNING) << "dump t " << t.Elapsed() << " " << val_type << " " << format << " path: " << path << " " << session->Signature();
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

void PsHandler::Load(const std::string& name, const std::string& path) {
  LOG(INFO) << "PsHandler::Load ...";
  /*
  try {
    Timer t;
    tools::MMap mmap;
    pb::DumpMeta meta;
    google::protobuf::TextFormat::ParseFromString(mmap.TextContent(path + ".meta"), &meta);
    pb::ValType val_type = meta.val_type();
    if (FLAGS_online_serving && val_type != pb::WEIGHTS) {
      throw std::runtime_error("Can not load " + pb::ValType_Name(val_type) + " from online serving ps!!! It must be WEIGHTS!!!");
    } else if (!FLAGS_online_serving && val_type != pb::PARAMS) {
      throw std::runtime_error("Can not load " + pb::ValType_Name(val_type) + " from offline training ps!!! It muse be PARAMS!!!");
    }
    meta.set_path(path);
    std::vector<std::string> model_defs;
    for (auto& def: meta.model_defs()) {
      model_defs.push_back(def.DebugString());
    }
    // FIXME: dumped model should also be serialized binary string 
    auto names = model_manager_->Create(name, model_defs, false);
    auto session = GetSession(names);
    session->Load(meta);
    session->ClearModels();
    LOG(WARNING) << "load t " << t.Elapsed() << " " << name << " path: " << path;
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception: " << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " unknown exception.";
    throw;
  }
  */
} // Load 

void PsHandler::RestoreModels() {  
  LOG(INFO) << "PsHandler::RestoreModels ...";
  /*
  Timer t;
  pb::CkptMeta ckptmeta;
  try {
    tools::MMap mmap;
    google::protobuf::TextFormat::ParseFromString(mmap.TextContent(FLAGS_ckpt_summarypath), &ckptmeta);
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception: " << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " unknown exception,";
    throw;
  }
  */
} // RestoreModels 

void PsHandler::SetStreamingUpdate(const std::vector<std::string>& names, const bool streaming_update) {
  LOG(INFO) << "PsHandler::SetStreamingUpdate ...";
  /*
  try {
    Timer t;
    auto session = GetSession(names);
    session->SetStreamingUpdate(streaming_update);
    session->ClearModels();
    LOG(WARNING) << "set streaming t " << t.Elapsed() << " " << session->Signature();
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

void PsHandler::ListModels(std::vector<ModelInfo>& models) {
  LOG(INFO) << "PsHandler::ListModels ...";
  /*
  try {
    Timer t;
    model_manager_->ListModels(models);
    LOG(WARNING) << "list_models t " << t.Elapsed();
  } catch (std::exception& e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception: " << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " unknown exception";
    throw;
  }
  */
} // ListModels

void PsHandler::Move(const std::string& from_name, const std::string& to_name, const std::string& backup_name) {
  /*
  try {
    Timer t;
    model_manager_->Move(from_name, to_name, backup_name);
    LOG(WARNING) << "move t=" << t.Elapsed();
  } catch (std::exception& e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception: " << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " Unknown exception";
    throw;
  }
  */
} // Move

void PsHandler::ModelDef(std::string& model_def, const std::string& name, const bool pb_binary) {
  /*
  try {
    Timer t;
    model_manager_->ModelDef(model_def, name, pb_binary);
    LOG(WARNING) << "model_def t=" << t.Elapsed() << " " << name;
  } catch (std::exception& e) {
    LOG(ERROR) << __FUNCTION__ << " std::exception: " << e.what();
    throw;
  } catch (...) {
    LOG(ERROR) << __FUNCTION__ << " unknown exception";
    throw;
  }
  */
}

void PsHandler::ServingType(std::string& serving_type) {
  try {
    Timer t;
    if (FLAGS_online_serving) {
      serving_type = "online";
    } else {
      serving_type = FLAGS_serving_type;
    }
    LOG(WARNING) << "serving_type t " << t.Elapsed(); 
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
    if (!FLAGS_online_serving) {
      for (auto& cmd: cmds) {
        int ret = openmi::Exec(cmd, outputs);
        if (ret != 0) break;
      }
    }
    LOG(WARNING) << "exec_shell t " << t.Elapsed();
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
