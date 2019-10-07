#ifndef OPENMI_PS_MODEL_H_
#define OPENMI_PS_MODEL_H_

#include "openmi/idl/proto/graph.pb.h"
using namespace openmi;

namespace openmi {
namespace ps {

class Model {
public: 
  Model(proto::GraphDef& gdef): gdef_(gdef) {}

  int Init();

  inline std::string Name() {
    return model_name_;
  }

  ~Model() {}

private:
  proto::GraphDef gdef_;
  std::unordered_map<int, std::shared_ptr<proto::internal::ModelWeightPtr>> model_weight_schema_;
  std::string model_name_;

}; // class Model

} // namespace ps
} // namespace openmi
#endif // OPENMI_PS_MODEL_H_
