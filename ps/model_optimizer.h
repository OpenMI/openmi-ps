#ifndef OPENMI_PS_MODEL_OPTIMIZER_H_
#define OPENMI_PS_MODEL_OPTIMIZER_H_

#include <string>
#include "optimizer.h"
#include "ps/smart_ptr.h"
using namespace openmi;

namespace openmi {
namespace ps {

struct OptimizerGroup {
  OptimizerGroup() {}

  ~OptimizerGroup() {
    optimizer_group.clear();
  }

  std::vector<BaseOptimizerPtr> optimizer_group;
}; // class OptimizerUnit
typedef std::shared_ptr<OptimizerGroup> OptimizerGroupPtr;

// manage model param update that callback optimizer
class ModelOptimizer {
public: 
  ModelOptimizer(ModelWeightSchemaPtrMapper& model_weight_schame);
  ~ModelOptimizer();

  int Init();

  OptimizerGroupPtr GetOptimizerGroup(int field);

private: 
  ModelWeightSchemaPtrMapper model_weight_schema_;
  std::unordered_map<int, OptimizerGroupPtr> optimizer_map_;
}; // class ModelOptimizer

typedef std::shared_ptr<ModelOptimizer> ModelOptimizerPtr;

} // namespace ps
} // namespace openmi
#endif // OPENMI_PS_MODEL_OPTIMIZER_H_