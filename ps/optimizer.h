#ifndef OPENMI_CORE_ENGINE_OPTIMIZER_OPTIMIZER_H_
#define OPENMI_CORE_ENGINE_OPTIMIZER_OPTIMIZER_H_

#include <memory>
#include "openmi/idl/proto/optimizer.pb.h"
using namespace openmi;

namespace openmi {

class BaseOptimizer {
public: 
  BaseOptimizer() {}
  ~BaseOptimizer() {}

  virtual int Compute(float* w, 
                     float g, 
                     char* opti_data, 
                     int& opti_bytes_offset,
                     proto::OptimizerConfig& optimizer_config) = 0;
}; // class BaseOptimzier

typedef std::shared_ptr<BaseOptimizer> BaseOptimizerPtr;

} // namespace openmi
#endif // OPENMI_CORE_ENGINE_OPTIMIZER_OPTIMIZER_H_