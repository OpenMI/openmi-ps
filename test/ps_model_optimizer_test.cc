#include <iostream> 
#include "openmi/idl/proto/optimizer.pb.h"
#include "openmi/idl/proto/engine.pb.h"
#include "openmi/idl/proto/communication.pb.h"
using namespace openmi;

typedef std::shared_ptr<proto::internal::ModelWeightSchema> ModelWeightSchemaPtr;

class ModelOptimizer {
public:
  void Update(proto::OptimizerConfig& config, ModelWeightSchemaPtr& weight_schema, void* weight, void* opti_data, const proto::comm::ValueList& grad);
protected: 
  bool is_gradient_clipping_;
}; // class BaseOptimizer

class FtrlOptimizer {
private: 
  float n_;
  float z_;
};
