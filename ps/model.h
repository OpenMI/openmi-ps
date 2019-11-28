#ifndef OPENMI_PS_MODEL_H_
#define OPENMI_PS_MODEL_H_

#include "openmi/idl/proto/graph.pb.h"
#include "openmi/idl/proto/engine.pb.h"
#include "ps/model_shard.h"
#include "ps/model_optimizer.h"
#include "smart_ptr.h"

using namespace openmi;
using namespace openmi::ps;

namespace openmi {
namespace ps {

static int shard_num_ = 8;

class Model {
public: 
  Model(proto::GraphDef& gdef): gdef_(gdef), model_optimizer_(nullptr) {}

  int Init();

  void SaveModelDef(const std::string& path);

  inline std::string Name() {
    return gdef_.name();
  }

  inline void SetName(const std::string& name) {
    gdef_.set_name(name);
  }

  inline proto::GraphDef& Def() {
    return gdef_;
  }

  ModelShardPtr GetModelShard(int shard_index) {
    if (model_shards_.size() <= shard_index) {
      throw std::runtime_error("shard_index out of range. index:" + std::to_string(shard_index));
    }
    return model_shards_.at(shard_index);
  }

  ~Model() {}

private:
  proto::GraphDef gdef_;
  // column_id/node_id -> weight schema  
  ModelWeightSchemaPtrMapper model_weight_schema_;
  ModelOptimizerPtr model_optimizer_;
  std::vector<ModelShardPtr> model_shards_;
}; // class Model

typedef std::shared_ptr<Model> ModelPtr;

} // namespace ps
} // namespace openmi
#endif // OPENMI_PS_MODEL_H_
