#include "ps/ps_session.h"
using namespace openmi;

namespace openmi {
namespace ps {

static int SHARD_NUM = 8;

void PsSession::Pull(std::vector<std::string>& _return, 
                     const std::vector<std::string>& reqs, 
                     const std::string& value_type, 
                     const std::string& req_id) {
  _return.resize(SHARD_NUM);
  CHECK(reqs.size() == SHARD_NUM);
  int key_count = 0;

  #pragma omp parallel for num_threads(SHARD_NUM) reduction(+:key_count)
  for (int i = 0; i < reqs.size(); ++i) {
    proto::comm::CommData req_data;
    req_data.ParseFromString(reqs[i]);
    key_count = req_data.keys_size();

    auto shard = model_->GetModelShard(i);
    proto::comm::CommData rsp_data;
    shard->Pull(req_data, rsp_data);
    rsp_data.SerializeToString(&_return[i]);
  }
}

void PsSession::Push(const std::vector<std::string>& reqs, 
                     const std::string& value_type, 
                     const std::string& req_id) {
  CHECK(reqs.size() == SHARD_NUM);
  int key_count = 0;

  #pragma omp parallel for num_threads(SHARD_NUM) reduction(+:key_count)
  for (int i = 0; i < reqs.size(); ++i) {
    proto::comm::CommData req_data;
    req_data.ParseFromString(reqs[i]);
    key_count = req_data.keys_size();
    
    auto shard = model_->GetModelShard(i);
    shard->Push(req_data, value_type);
  }
}

void PsSession::Dump(const std::string& path, proto::ModelValueType value_type, const std::string& format, const bool dump_zero) {
  // todo parallel
  for (int i = 0; i < SHARD_NUM; ++i) {
    auto shard = model_->GetModelShard(i);
    shard->Dump(path, value_type, format, dump_zero);
  }
  model_->SaveModelDef(path);
}

void PsSession::Load(const std::string& path, proto::ModelValueType value_type, const std::string& format) {
  // todo parallel
  for (int i = 0; i < SHARD_NUM; ++i) {
    auto shard = model_->GetModelShard(i);
    shard->Load(path);
  }
}

} // namespace ps
} // namespace openmi