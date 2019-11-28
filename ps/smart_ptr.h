#ifndef OPENMI_PS_SMART_PTR_H_
#define OPENMI_PS_SMART_PTR_H_

#include <memory>
#include <unordered_map>

#include "openmi/idl/proto/communication.pb.h"
#include "openmi/idl/proto/engine.pb.h"
using namespace openmi;

namespace openmi {

typedef std::shared_ptr<proto::comm::ValueList> ValueListPtr;
typedef std::shared_ptr<proto::internal::ModelWeightSchema> ModelWeightSchemaPtr;
typedef std::unordered_map<int, ModelWeightSchemaPtr> ModelWeightSchemaPtrMapper;
typedef std::shared_ptr<proto::internal::WeightSchema> WeightSchemaPtr;

} // namespace openmi
#endif // OPENMI_PS_SMART_PTR_H_