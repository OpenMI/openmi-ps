#include "openmi/idl/proto/graph.pb.h"
#include "openmi/core/engine/session.h"
#include "openmi/core/engine/model_parser.h"
#include "base/logging.h"
#include "base/protobuf_op.h"
#include "ps/internal/kv_storer.h"
using namespace openmi;

int type_size = sizeof(float);

class FtrlOptimizer {
private: 
  float n_;
  float v_;
};

void init_model_weight(ps::KVStorer& kv, uint64_t fid, std::shared_ptr<proto::internal::ModelWeightSchema>& weight_schema) {
  int total_size = 0;
  for (int i = 0; i < weight_schema->weight_schema_size(); ++i) {
    auto& weight_schema_ith = weight_schema->weight_schema(i);
    // ith weight size
    total_size += weight_schema_ith.weight_size() * type_size;
    // ith optimizer size 
    total_size += weight_schema_ith.weight_size() * sizeof(FtrlOptimizer);
  }

  LOG(INFO) << "fid: " << fid << ", total_size:" << total_size;
  bool created = false;
  void* data = kv.GetOrCreate(fid, total_size, created);
  CHECK(data != nullptr);
}

int main(int argc, char** argv) {
  const char* file = "../openmi/unittest/conf/wide_and_deep_graph_demo.conf";
  proto::GraphDef gdef;
  if (ProtobufOp::LoadObjectFromPbFile<proto::GraphDef>(file, &gdef) != 0) {
    LOG(ERROR) << "load graph def proto file failed.";
    return -1;
  }
  LOG(INFO) << "load graph file done.";

  Executor exec(gdef);
  openmi::Graph* g = exec.GetGraph();
  LOG(INFO) << "source nodes size: " << g->source_nodes().size();
  LOG(INFO) << "global topo nodes size: " << g->global_topo_nodes().size();
  
  std::unordered_map<int, std::shared_ptr<proto::internal::ModelWeightSchema>> model_weight_schema;
  ModelParser::CreateModelWeightSchema(g, model_weight_schema);
  LOG(INFO) << "model_weight_schema.size: " << model_weight_schema.size();

  auto iter = model_weight_schema.begin();
  while (iter != model_weight_schema.end()) {
    LOG(INFO) << "column id:" << iter->first << ", model weight schema:\n" << iter->second->DebugString();
    iter++;
  }

  return 0;
}
