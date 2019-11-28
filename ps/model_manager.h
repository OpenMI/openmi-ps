#ifndef OPENMI_PS_MODEL_MANAGER_H_
#define OPENMI_PS_MODEL_MANAGER_H_

#include <mutex>
#include <string>
#include <unordered_map>

#include "ps/model.h"
using namespace openmi::ps;

namespace openmi {
namespace ps {

class ModelManager {
public:
  ModelManager() {}
  ~ModelManager() {}

  std::string Create(const std::string& graph_def, bool pb_binary);

  void GetModelDef(std::string& _return, const std::string& name, const bool pb_binary);

  ModelPtr GetModel(const std::string& name);

  void ListModels(std::vector<std::string>& models);

  bool IsExist(const std::string& name);
  
  void Drop(const std::string& name);
  void Move(const std::string& from, const std::string& to, const std::string& bak);

private: 
  int DeleteModel(const std::string& name);
  int AddModel(const std::string& name, ModelPtr& model);

private: 
  std::unordered_map<std::string, ModelPtr> model_map_;
  std::mutex mu_;
};

typedef std::shared_ptr<ModelManager> ModelManagerPtr; 

} // namespace ps
} // namespace openmi
#endif // OPENMI_PS_MODEL_MANAGER_H_