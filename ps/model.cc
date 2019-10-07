#include "model.h"
#include "openmi/core/engine/model_parser.h"
#include "openmi/core/graph/graph.h"
#include "openmi/core/graph/graph_constructor.h"

namespace openmi {
namespace ps {

int Model::Init() {
  // 1. 解析gdef_，得到参数schema，用于ModelShard初始化；
  auto g = std::make_shared<openmi::Graph>();
  Status status = ConvertGraphDefToGraph(&gdef_, g.get());
  // 2. 生成model weight schema
  ModelParser::CreateModelWeightSchema(g.get(), model_weight_schema_);
  LOG(INFO) << "model_weight_schema: " << model_weight_schema_.size();

  model_name_ = g->name();
  return 0;
}

} // namespace ps
} // namespace openmi