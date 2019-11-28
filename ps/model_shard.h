#ifndef OPENMI_PS_MODEL_SHARD_H_
#define OPENMI_PS_MODEL_SHARD_H_

#include <memory>
#include <unordered_map>
#include "openmi/idl/proto/communication.pb.h"
#include "openmi/idl/proto/engine.pb.h"
#include "openmi/idl/proto/model.pb.h"
#include "ps/internal/kv_storer.h"
#include "ps/model_optimizer.h"
#include "smart_ptr.h"
using namespace openmi;

namespace openmi {
namespace ps {

class ModelShard {
public: 
  ModelShard(int shard_index, 
             const std::string& name, 
             ModelWeightSchemaPtrMapper& model_weight_schema, 
             ModelOptimizerPtr& model_optimizer);

  ~ModelShard();

  int Pull(const proto::comm::CommData& req, proto::comm::CommData& rsp);
  int Push(const proto::comm::CommData& req, const std::string& value_type);

  int Dump(const std::string& path, proto::ModelValueType value_type, const std::string& format, const bool dump_zero);
  int Load(const std::string& path);
private: 

  int FillWeight(char* weight_data, const proto::comm::ValueList& value, 
                 ModelWeightSchemaPtr& weight_schema);

  void InitWeightUsingUniformDistribution(float* data, proto::comm::ValueList* vals, 
                                          int size, int& offset, proto::InitailizationParam& init_params);

  void InitWeightUsingNormalDistribution(float* data, proto::comm::ValueList* vals, 
                                         int size, int& offset, proto::InitailizationParam& init_params);

  void UpdateWeight(char* weight_data, 
                    char* optimizer_data, 
                    const proto::comm::ValueList& grad_data,
                    ModelWeightSchemaPtr& weight_schema, 
                    OptimizerGroupPtr& optimizer);

  void UpdateWeight(float* weight, int weight_offset, int weight_size, char* opti_data, int& opti_bytes_offset, 
                    const proto::comm::ValueList& gradient_data, 
                    proto::internal::WeightSchema& weight_schema,
                    BaseOptimizerPtr& optimizer);

  int DumpToLocalPath(const std::string& dirpath, KVStorer& kv_storer, proto::ModelValueType value_type, int batch = 10);
  void SaveDumpModelMeta(const std::string& dirpath, proto::ModelValueType value_type);
  
  int LoadFormLocalPath(proto::DumpModelMeta& meta, proto::ModelValueType value_type, KVStorer& kv_storer);
  int FillToKVStorer(proto::StoredModel& stored_model, KVStorer& kv_storer);



private: 
  KVStorer weight_data_;              // model weight data
  KVStorer weight_optimizer_data_;    // model weight middle variable. optimizer data
  int shard_index_;
  std::string model_name_;
  bool online_ps_;
  ModelWeightSchemaPtrMapper& model_weight_schema_;
  ModelOptimizerPtr& model_optimizer_;
}; // class ModelShard

typedef std::shared_ptr<ModelShard> ModelShardPtr;

} // naemspace ps
} // namespace openmi
#endif // OPENMI_PS_MODEL_SHARD_H_