#include "base/logging.h"
#include "openmi/core/engine/model_parser.h"
#include "openmi/core/graph/graph.h"
#include "openmi/core/graph/graph_constructor.h"
#include "openmi/core/framework/executor.h"
#include "ps/model.h"
#include "base/write_op.h"

namespace openmi {
namespace ps {

int Model::Init() {
  Executor exec(gdef_);
  if (exec.Init() != 0) {
    LOG(ERROR) << "executor init failed.";
    return -1;
  }
  // init model weight schema
  if (ModelParser::CreateModelWeightSchema(exec.GetGraph(), model_weight_schema_) != 0) {
    LOG(ERROR) << __FUNCTION__ << " Error: create model weight schema.";
    return -1;
  }
  LOG(INFO) << "model_weight_schema size: " << model_weight_schema_.size();

  // init model optimizer
  model_optimizer_ = std::make_shared<ModelOptimizer>(model_weight_schema_);
  if (model_optimizer_ == nullptr) {
    LOG(ERROR) << __FUNCTION__ << "model optimizer new failed.";
    return -1;
  }
  if (model_optimizer_->Init() != 0) {
    LOG(ERROR) << __FUNCTION__ << "model optimizer init failed.";
    return -1;
  }

  // init model shard
  CHECK(shard_num_ > 0) << "shard_num must be > 0";
  for (int i = 0; i < shard_num_; ++i) {
    auto shard = std::make_shared<ModelShard>(
      i, Name(), model_weight_schema_, model_optimizer_);
    model_shards_.push_back(shard);
  }

  return 0;
}

void Model::SaveModelDef(const std::string& path) {
  auto filepath = path + "/" + Name() + ".graph";
  Write write(filepath.c_str());
  std::string graph_str = gdef_.DebugString();
  write.write(graph_str.c_str(), graph_str.size());
  write.close();
  // todo md5
}

} // namespace ps
} // namespace openmi
