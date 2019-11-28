#include "model_shard.h"
#include "base/file_op.h"
#include "base/protobuf_op.h"
#include "base/random.h"
#include "base/read_op.h"
#include "base/write_op.h"
using namespace openmi;

namespace openmi {
namespace ps {

using SaveHandler = std::function<void(proto::StoredModel& stored_model)>;

ModelShard::ModelShard(int shard_index, 
                       const std::string& name, 
                       ModelWeightSchemaPtrMapper& model_weight_schema, 
                       ModelOptimizerPtr& model_optimizer)
  : shard_index_(shard_index), 
    model_name_(name),
    model_weight_schema_(model_weight_schema),
    model_optimizer_(model_optimizer),
    online_ps_(false) { }

ModelShard::~ModelShard() {
}

int ModelShard::Pull(const proto::comm::CommData& req, proto::comm::CommData& rsp) {
  DLOG(INFO) << __FUNCTION__ << " pull begin";
  CHECK(req.keys_size() == req.fields_size());
  for (int i = 0; i < req.keys_size(); ++i) {
    auto key = req.keys(i);
    auto field_id = req.fields(i);
    rsp.add_keys(key);
    rsp.add_fields(field_id);
    auto* val_list = rsp.add_vals();
    auto it = model_weight_schema_.find(field_id);
    if (it == model_weight_schema_.end()) {
      LOG(ERROR) << "field '" << field_id 
                 << "' not in model weight schema. model:" << model_name_;
      rsp.add_lens(0);
      continue;
    }
    // init weight_data_
    void* data = nullptr;
    bool created = false;
    auto weight_schema = model_weight_schema_.at(field_id);
    if (online_ps_) {
      data = weight_data_.Get(key, weight_schema->total_weight_bytes());
    } else {
      data = weight_data_.GetOrCreate(
        key, weight_schema->total_weight_bytes(), created);
      if (created) {
        weight_optimizer_data_.GetOrCreate(
          key, weight_schema->total_weight_optimizer_bytes(), created);
      }
    }

    rsp.add_lens(weight_schema->total_weight_size());

    if (!data) {
      // 线上默认0；
      // todo pb优化性能
      for (int k = 0; k < weight_schema->total_weight_size(); ++k) {
        val_list->add_val(0);
      }
      return 0;
    }

    // 如果created=true, 则需要初始化W权重，使用ModelWeightSchema中的方法；
    float* fdata = reinterpret_cast<float*>(data);
    if (created) {
      int offset = 0;
      for (int j = 0; j < weight_schema->weight_schema_size(); ++j) {
        int size = weight_schema->weight_schema(j).weight_size();
        auto optimizer = weight_schema->weight_schema(j).optimizer();
        auto init_method = optimizer.init_method();
        auto init_param = optimizer.init_param();
        switch (init_method) {
          case proto::DEFAULT: case proto::UNIFORM_DISTRIBUTION: 
          {
            InitWeightUsingUniformDistribution(fdata, val_list, size, offset, init_param);
            break;
          }
          case proto::NORMAL_DISTRIBUTION: 
          {
            InitWeightUsingNormalDistribution(fdata, val_list, size, offset, init_param);
            break;
          }
          default: 
          {
            LOG(ERROR) << "unknown init method:" << proto::InitailizationMethod_Name(init_method);
            return -1;
          }
        }
      }
    } else {
      for (int j = 0; j < weight_schema->total_weight_size(); ++j) {
        val_list->add_val(*(fdata+j));
      }
    }
  }
  DLOG(INFO) << __FUNCTION__ << " pull done";
  return 0;
}

int ModelShard::Push(const proto::comm::CommData& req, const std::string& value_type) {
  LOG(INFO) << __FUNCTION__ << " value type: " << value_type;
  // TODO 如果value_type是weight而不是gradient，如何赋值给weight_data？？？
  if (req.keys_size() == 0) {
    return 0;
  }
  CHECK(req.keys_size() == req.fields_size());
  LOG(INFO) << __FUNCTION__ << " keys size:" << req.keys_size();

  for (int i = 0; i < req.keys_size(); ++i) {
    auto key = req.keys(i);
    auto field = req.fields(i);

    auto it = model_weight_schema_.find(field);
    if (it == model_weight_schema_.end()) {
      LOG(ERROR) << __FUNCTION__ << ". field not in model weight schema. "
        << " field: " << field << ", model: " << model_name_;
      return -1;
    }
    auto& weight_schema = it->second;
    char* data = weight_data_.Get(key, weight_schema->total_weight_bytes());
    if (data == nullptr) {
      LOG(WARN) << __FUNCTION__ << " weight key not in weight data. "
        << "key: " << key << ", model: " << model_name_;
      continue;
    }
    char* opti_data = weight_optimizer_data_.Get(
      key, weight_schema->total_weight_optimizer_bytes());
    if (opti_data == nullptr) {
      LOG(WARN) << __FUNCTION__ << ". weight key not in weight optimizer data. " 
        << "key:" << key << ", model:" << model_name_;
      continue;
    }

    auto value = req.vals(i);
    if (value_type == "WEIGHT") {
      // 写模型参数（应用于模型在线更新或离线push模型）
      FillWeight(data, value, weight_schema);
    } else if (value_type == "GRADIENT") {
      auto optimizer = model_optimizer_->GetOptimizerGroup(field);
      if (!optimizer) {
        LOG(ERROR) << "optimizer not exists. field:" << field << ", model: " << model_name_;
        return -1; 
      }
      UpdateWeight(data, opti_data, value, weight_schema, optimizer);
    } else {
      LOG(ERROR) << "Unknown value type: " << value_type;
      return -1;
    }
  }
  return 0;
}

static std::string StoredModelPath(const std::string& path, const std::string& model_name, proto::ModelValueType value_type, int shard_index) {
  std::string new_path = path + "/" + model_name + "." + proto::ModelValueType_Name(value_type) + "." + std::to_string(shard_index);
  return new_path;
}

static std::string DumpMetaPath(const std::string& path, const std::string& model_name) {
  std::string meta_path = path + "/" + model_name + ".meta";
  return meta_path;
}

int ModelShard::Dump(const std::string& path, proto::ModelValueType value_type, const std::string& format, const bool dump_zero) {
  proto::ModelValueType dump_meta_value_type = proto::WEIGHT;

  if (DumpToLocalPath(path, weight_data_, proto::WEIGHT) != 0) {
    LOG(ERROR) << __FUNCTION__ << " dump to local path failed. "
              << "dir path[" << path << ", value_type: WEIGHT";
    return -1;
  }

  if (value_type == proto::CHECKPOINT) {
    if (online_ps_) {
      LOG(WARN) << __FUNCTION__ << " online ps model have not 'CHECKPOINT' type.";
    } else {
      dump_meta_value_type = proto::CHECKPOINT;
      if (DumpToLocalPath(path, weight_optimizer_data_, proto::CHECKPOINT) != 0) {
        LOG(ERROR) << __FUNCTION__ << " dump to local path failed. "
                   << "dir path[" << path << ", value_type: CHECKPOINT";
        return -1;
      }
    }
  }

  if (shard_index_ == 0) {
    SaveDumpModelMeta(path, dump_meta_value_type);
  }
  
  return 0;
}

int ModelShard::Load(const std::string& path) {
  std::string meta_path = DumpMetaPath(path, model_name_);
  proto::DumpModelMeta model_meta;
  if (ProtobufOp::LoadObjectFromPbFile<proto::DumpModelMeta>(meta_path.c_str(), &model_meta) != 0) {
    LOG(ERROR) << "parse DumpModelMeta from file failed. meta_path: " << meta_path;
    return -1; 
  }

  if (LoadFormLocalPath(model_meta, proto::WEIGHT, weight_data_) != 0) {
    LOG(ERROR) << __FUNCTION__ << " load from load path failed. "
               << "value_type: WEIGHT, model: " << model_name_;
    return -1;
  }

  if (model_meta.value_type() == proto::CHECKPOINT) {
    if (online_ps_) {
      LOG(WARN) << __FUNCTION__ << " online ps model have not 'CHECKPOINT' type.";
    } else {
      if (LoadFormLocalPath(model_meta, proto::CHECKPOINT, weight_optimizer_data_) != 0) {
        LOG(ERROR) << __FUNCTION__ << " load from load path failed. "
               << "value_type: CHECKPOINT, model: " << model_name_;
        return -1;
      }
    }
  }
  return 0;
}

int ModelShard::LoadFormLocalPath(proto::DumpModelMeta& meta, proto::ModelValueType value_type, KVStorer& kv_storer) {
  std::string filepath = StoredModelPath(meta.path(), model_name_, value_type, shard_index_);
  // todo 需要从${model_name}.meta_info中获取format等信息，参数去除
  proto::StoredModel loaded_model;
  Read* read = new Read(filepath.c_str(), true);
  std::string buf; 
  buf.reserve(1024*1024);
  size_t size;
  while (read->read(reinterpret_cast<char*>(&size), sizeof(size))) {
    buf.resize(size);
    if (read->read(const_cast<char*>(buf.c_str()), size)) {
      loaded_model.ParseFromString(buf);
      if (FillToKVStorer(loaded_model, kv_storer) != 0) {
        LOG(ERROR) << "fill to kv storer failed.";
        return -1;
      }
      loaded_model.Clear();
    }
  }
  delete read;
  LOG(INFO) << __FUNCTION__ << " done. shard_index:" << shard_index_;
  return 0;
}

int ModelShard::FillToKVStorer(proto::StoredModel& stored_model, KVStorer& kv_storer) {
  for (auto i = 0; i < stored_model.keys_size(); ++i) {
    auto key = stored_model.keys(i);
    auto param = stored_model.params(i);
    kv_storer.Update(key, const_cast<char*>(param.data().data()), param.byte_size());
  }
  return 0;
}

int ModelShard::FillWeight(char* weight_data, const proto::comm::ValueList& value, 
               ModelWeightSchemaPtr& weight_schema) {
  if (value.val_size() != weight_schema->total_weight_size()) {
    LOG(ERROR) << "weight_data.val_size() != weight_schema->total_weight_size(). "
               << value.val_size() << " vs " << weight_schema->total_weight_size();
    return -1;
  }

  float* weight = reinterpret_cast<float*>(weight_data);
  int offset = 0;
  for (int i = 0; i < value.val_size(); ++i) {
    LOG(INFO) << "before *(weight + " << offset << "): " << *(weight+offset) << ", value.val(i): " << value.val(i);
    *(weight + offset) = value.val(i);
    LOG(INFO) << "after *(weight + " << offset << "): " << *(weight+offset);
    offset++;
  }

  return 0;
}

void ModelShard::InitWeightUsingNormalDistribution(float* data, 
  proto::comm::ValueList* vals, int size, int& offset, proto::InitailizationParam& init_params) {
  for (int i = 0; i < size; ++i) {
    auto w = NormalRandom(init_params.mean(), init_params.stddev());
    if (init_params.is_truncated() && std::abs(w) > init_params.threshold()) {
      w = init_params.threshold();
    }
    vals->add_val(w);
    *(data + offset) = w;
    offset ++;
  }
  LOG(INFO) << __FUNCTION__ << " vals.size:" << vals->val_size() << ", value:\n" << vals->DebugString();
}

void ModelShard::InitWeightUsingUniformDistribution(float* data, 
  proto::comm::ValueList* vals, int size, int& offset, proto::InitailizationParam& init_params) {
  for (int i = 0; i < size; ++i) {
    auto w = UniformRandom(init_params.min(), init_params.max());
    vals->add_val(w);
    *(data + offset) = w;
    offset ++;
  }
  LOG(INFO) << __FUNCTION__ << " vals.size:" << vals->val_size() << ", value:\n" << vals->DebugString();
}

void ModelShard::UpdateWeight(float* weight, int weight_offset, int weight_size, char* opti_data, int& opti_bytes_offset, 
                              const proto::comm::ValueList& gradient_data, 
                              proto::internal::WeightSchema& weight_schema,
                              BaseOptimizerPtr& optimizer) {
  auto optimizer_config = weight_schema.optimizer();                                
  for (int i = 0; i < weight_size; ++i) {
    auto& w = *(weight + weight_offset + i);
    auto g = gradient_data.val(weight_offset + i);

    LOG(INFO) << __FUNCTION__ << ". before i: " << i << ", opti_bytes_offset: " << opti_bytes_offset << ", w: " << w << ", g:" << g;
    optimizer->Compute(&w, g, opti_data, opti_bytes_offset, optimizer_config);
    LOG(INFO) << __FUNCTION__ << ". after i: " << i << ", opti_bytes_offset: " << opti_bytes_offset << ", w: " << w << ", g:" << g;
  }
}

void ModelShard::UpdateWeight(char* weight_data, 
                              char* optimizer_data, 
                              const proto::comm::ValueList& gradient_data, 
                              ModelWeightSchemaPtr& weight_schema, 
                              OptimizerGroupPtr& optimizer_group) {
  int weight_offset = 0;
  int opti_bytes_offset = 0;
  float* weight = reinterpret_cast<float*>(weight_data);
  char* opti = reinterpret_cast<char*>(optimizer_data);
  for (int i = 0; i < weight_schema->weight_schema_size(); ++i) {
    auto ith_weight_schema = weight_schema->weight_schema(i);
    int ith_weight_size = ith_weight_schema.weight_size();
    int ith_weight_opti_bytes = ith_weight_schema.weight_bytes();
    int ith_weight_opti_bytes_offset = ith_weight_schema.weight_optimizer_bytes_offset();
    CHECK(opti_bytes_offset == ith_weight_opti_bytes_offset) 
      << opti_bytes_offset << " vs " << ith_weight_opti_bytes_offset;

    auto optimizer = optimizer_group->optimizer_group[i];
    UpdateWeight(weight, weight_offset, ith_weight_size, opti, opti_bytes_offset, gradient_data, ith_weight_schema, optimizer);

    weight_offset += ith_weight_size;
  }
}

int ModelShard::DumpToLocalPath(const std::string& dirpath, KVStorer& kv_storer, proto::ModelValueType value_type, int batch) {
  if (FileOp::access_dir(dirpath.c_str())) { 
    if (!FileOp::is_dir(dirpath.c_str())) {
      throw std::runtime_error("directory path exist but it is file. path:" + dirpath);
    } else {
      LOG(INFO) << "model exists. backup it.";
      // todo 之前dump的模型，需要备份到 ${path}.bak, mkdir && mvdir
    }
  } else {
    LOG(WARN) << "directory path '" << dirpath << "' not exist. mkdir it.";
    if (!FileOp::mk_dir(dirpath.c_str())) {
      LOG(ERROR) << "create dump dir failed. path:" << dirpath;
      return -1;
    }
  }

  std::string filepath = StoredModelPath(dirpath, model_name_, value_type, shard_index_);
  Write* write = new Write(filepath.c_str(), true);

  SaveHandler handler = [&] (proto::StoredModel& stored_model) {
    std::string bin;
    stored_model.SerializeToString(&bin);
    size_t size = bin.size();
    write->write_binary(reinterpret_cast<char*>(&size), sizeof(size));
    write->write_binary(bin.c_str(), size);
  };
  
  int count = 0;
  proto::StoredModel stored_model;
  KVStorerHandler fn = [&](FID key, void* data, int size) {
    if (size == 0) {
      LOG(WARN) << "data.size == 0. key:" << key;
      return;
    }
    stored_model.add_keys(key);
    auto* param = stored_model.add_params();
    param->set_data(data, size);
    param->set_byte_size(size);
    
    count += 1;
    if (count == batch) {
      LOG(INFO) << "count == 10. stored_model.keys_size(): " << stored_model.keys_size();
      LOG(INFO) << "stored_model:" << stored_model.DebugString();
      handler(stored_model);
      stored_model.Clear();
      count = 0;
    }
  };

  kv_storer.ForEach(fn);
  handler(stored_model);
  stored_model.Clear();
  
  write->close();
  delete write;
  return 0;
}

void ModelShard::SaveDumpModelMeta(const std::string& path, proto::ModelValueType value_type) {
  proto::DumpModelMeta dump_meta;
  dump_meta.set_path(path);
  dump_meta.set_model_name(model_name_);
  dump_meta.set_value_type(value_type);

  std::string meta_path = DumpMetaPath(path, model_name_);
  Write* write = new Write(meta_path.c_str());
  std::string content = dump_meta.DebugString();
  write->write_line(content.c_str(), content.size());
  write->close();
  delete write;
}

} // namespace ps
} // namespace openmi
