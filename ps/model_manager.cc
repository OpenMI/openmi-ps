
#include <google/protobuf/text_format.h>
#include "base/logging.h"
#include "openmi/idl/proto/graph.pb.h"
#include "ps/model_manager.h"
using namespace openmi; 

namespace openmi {
namespace ps {

std::string ModelManager::Create(const std::string& graph_def, bool pb_binary) {
  std::lock_guard<std::mutex> lock(mu_);
  std::string _return("");
  proto::GraphDef gdef;
  if (pb_binary) {
    gdef.ParseFromString(graph_def);
  } else {
    LOG(WARN) << __FUNCTION__ <<  " Unsafe: parse graph def from text format!!!";
    google::protobuf::TextFormat::ParseFromString(graph_def, &gdef);
  }

  auto graph_name = gdef.name();
  if (graph_name.empty()) {
    LOG(ERROR) << "graph def 'name' field is empty. please check graph_def.\n"
               << gdef.ShortDebugString();
    return _return;
  }
  if (model_map_.find(graph_name) != model_map_.end()) {
    LOG(ERROR) << __FUNCTION__ << " model '" << graph_name 
               << "' has already exists. please check duplication name.";
    return _return;
  }

  auto model = std::make_shared<Model>(gdef); 
  if (model->Init() != 0) {
    LOG(ERROR) << "model init failed. name:" << graph_name 
               << ", graph def:\n" << gdef.ShortDebugString();
    return _return; 
  }
  _return = graph_name;
  model_map_.insert({_return, model});
  return _return;
}

void ModelManager::GetModelDef(std::string& _return, const std::string& name, const bool pb_binary) {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = model_map_.find(name);
  if (it == model_map_.end()) {
    LOG(ERROR) << __FUNCTION__ << " model def not exists. name:" << name;
    return;
  }
  auto graph_def = it->second->Def();
  if (pb_binary) {
    graph_def.SerializeToString(&_return);
  } else {
    LOG(WARN) << __FUNCTION__ << " Unsafe: graph def to text format!!!";
    google::protobuf::TextFormat::PrintToString(graph_def, &_return);
  }
}

ModelPtr ModelManager::GetModel(const std::string& name) {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = model_map_.find(name);
  if (it == model_map_.end()) {
    LOG(ERROR) << __FUNCTION__ << " model '" << name << "' not exist!";
    return nullptr;
  }
  return it->second;
}

void ModelManager::ListModels(std::vector<std::string>& models) {
  std::unique_lock<std::mutex> lock(mu_);
  for (auto it = model_map_.begin(); it != model_map_.end(); ++it) {
    models.push_back(it->first);
  }
}

bool ModelManager::IsExist(const std::string& name) {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = model_map_.find(name);
  if (it == model_map_.end()) {
    return false;
  }
  return true;
}

void ModelManager::Drop(const std::string& name) {
  std::lock_guard<std::mutex> lock(mu_);
  if (DeleteModel(name) != 0) {
    LOG(ERROR) << __FUNCTION__ << " delete model failed. name:" << name;
    return;
  }
  LOG(INFO) << "model_map.size: " << model_map_.size();
}

// 1. to -> bak; 2. from -> to;
void ModelManager::Move(const std::string& from, const std::string& to, const std::string& bak) {
  std::unique_lock<std::mutex> lock(mu_);
  if (model_map_.find(bak) != model_map_.end()) {
    DeleteModel(bak);
  }

  if (model_map_.find(from) == model_map_.end()) {
    LOG(ERROR) << "from model '" + from + "' not exists. it can't be move model.";
    return;
  }

  if (model_map_.find(to) != model_map_.end()) {
    AddModel(bak, model_map_[to]);
    model_map_[bak]->SetName(bak);
    DeleteModel(to);
  }

  AddModel(to, model_map_[from]);
  model_map_[to]->SetName(to);
}

int ModelManager::DeleteModel(const std::string& name) {
  auto it = model_map_.find(name);
  if (it == model_map_.end()) {
    LOG(ERROR) << __FUNCTION__ << " model '" << name << "' not exists!";
    return -1;
  }
  model_map_.erase(name);
  return 0;
}

int ModelManager::AddModel(const std::string& name, ModelPtr& model) {
  if (model_map_.find(name) != model_map_.end()) {
    LOG(ERROR) << "model '" << name << "' exists. it can't add model.";
    return -1;
  }
  model_map_.insert({name, model});
  return 0;
}

} // namespace ps
} // namespace openmi