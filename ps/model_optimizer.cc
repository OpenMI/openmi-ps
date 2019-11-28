#include "base/logging.h"
#include "ps/model_optimizer.h"
#include "ftrl.h"
using namespace openmi;

namespace openmi {
namespace ps {

ModelOptimizer::ModelOptimizer(ModelWeightSchemaPtrMapper& model_weight_schema)
  : model_weight_schema_(model_weight_schema) {}

ModelOptimizer::~ModelOptimizer() {}

int ModelOptimizer::Init() {
  int total_num = 0;
  auto it = model_weight_schema_.begin();
  while (it != model_weight_schema_.end()) {
    int id = it->first;
    auto& weight_schemas = it->second;
    LOG(INFO) << "id:" << id << ", weight schemas:\n" << weight_schemas->DebugString();
    auto optimizer_group = std::make_shared<OptimizerGroup>();
    for (int i = 0; i < weight_schemas->weight_schema_size(); ++i) {
      auto& optimizer_conf = weight_schemas->weight_schema(i).optimizer();
      DLOG(INFO) << "weight schema:\n" << weight_schemas->weight_schema(i).DebugString();
      auto opti_type = optimizer_conf.type();
      // TODO 根据opti_type得到具体的优化器，使用工厂模式实现，输入是enum
      BaseOptimizerPtr optimizer = std::make_shared<FtrlOptimizer>();
      optimizer_group->optimizer_group.emplace_back(optimizer);
      total_num++;
    }
    
    if (optimizer_map_.find(id) != optimizer_map_.end()) {
      LOG(ERROR) << "optimizer has already exists. id:" << id;
      return -1;
    }
    optimizer_map_.insert({id, optimizer_group});

    it++;
  }
  LOG(INFO) << __FUNCTION__ << " done. group size:" 
    << optimizer_map_.size() << ", optimizer total num:" << total_num;
  return 0;
}

OptimizerGroupPtr ModelOptimizer::GetOptimizerGroup(int field) {
  return optimizer_map_.at(field);
}

} // namespace ps
} // namespace openmi