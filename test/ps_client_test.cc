#include <string>
#include <vector>

#include "base/logging.h"
#include "base/protobuf_op.h"
#include "base/random.h"
#include "openmi/core/distribute_runtime/thrift_client_wrapper.h"
#include "openmi/core/framework/executor.h"
#include "openmi/core/engine/model_parser.h"
#include "openmi/idl/proto/graph.pb.h"
#include "openmi/idl/proto/engine.pb.h"
#include "openmi/idl/proto/communication.pb.h"
using namespace openmi;
#include "openmi/gen-cpp/Ps.h"   // PsClient 
using namespace openmi::thrift;

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <gflags/gflags.h>

DEFINE_string(hosts, "localhost", "ps cluster hostname list");
DEFINE_string(ports, "4321", "ps port list");
DEFINE_int32(conn_timeout, 100, "connection timeout");
DEFINE_int32(timeout, 500, "send/recv timeout");


typedef std::unordered_map<int, std::shared_ptr<proto::internal::ModelWeightSchema>> SchemaType;

void init_log(const char* name, const char* log_dir) {
  InitLogging(name, log_dir);
  g_alsologtostderr = false;
  g_log_severity = INFO;
  LOG(INFO) << "init logging done. name: " << name << ", logging dir: " << log_dir << ", g_log_dir: " << g_log_dir;
}

const char* graph_file = "../openmi/unittest/conf/wide_and_deep_graph_demo.conf";
std::string dump_and_load_path = "log/stored_model.test";
typedef std::shared_ptr<PsClient> PsClientPtr;


void PsHandler_Create(PsClientPtr& client, proto::GraphDef& gdef) {
  std::string _return;

  std::string graph_def;
  
  graph_def = gdef.DebugString();
  client->Create(_return, graph_def, false);
  LOG(INFO) << "pb_binary=false, return model name: " << _return;
  /*
  gdef.SerializeToString(&graph_def);
  client->Create(_return, graph_def, true);
  LOG(INFO) << "pb_binary=true, return model name: " << _return;
  */
}

void PsHandler_ModelDef(PsClientPtr& client, proto::GraphDef& gdef) {
  std::string _return;
  auto graph_name = gdef.name();
  client->ModelDef(_return, graph_name, true);

  proto::GraphDef model_def;
  model_def.ParseFromString(_return);
  LOG(INFO) << "get model def:\n" << model_def.DebugString();
}

void GenerateReq(int reqs_num, std::vector<proto::comm::CommData>& reqs) {
  int num = 10;
  for (int i = 0; i < reqs_num; ++i) {
    proto::comm::CommData req;
    for (int j = 0; j < num+i; ++j) {
      req.add_keys((i+1)*10000 + 123 + j);
      req.add_fields((j+1)%3 + 1);
    }
    reqs.emplace_back(req);
  }
}

void PsHandler_Pull(PsClientPtr& client, proto::GraphDef& gdef) {
  std::vector<proto::comm::CommData> req_comm_datas;
  GenerateReq(8, req_comm_datas);
  std::vector<std::string> reqs;
  reqs.resize(req_comm_datas.size());
  for (int i = 0; i < req_comm_datas.size(); ++i) {
    req_comm_datas[i].SerializeToString(&reqs[i]);
  }

  std::string value_type("WEIGTH_TEST");
  std::string req_id("req_id_1000");
  std::vector<std::string> rsps;
  client->Pull(rsps, gdef.name(), reqs, value_type, req_id);
  LOG(INFO) << "rsps.size: " << rsps.size();
  for (int i = 0; i < rsps.size(); ++i) {
    proto::comm::CommData rsp;
    rsp.ParseFromString(rsps[i]);
    LOG(INFO) << "rsp[" << i << "]:\n" << rsp.DebugString();
  }
}

void PsHandler_Push(PsClientPtr& client, proto::GraphDef& gdef, const std::string value_type, SchemaType model_weight_schema) {
  std::vector<proto::comm::CommData> req_comm_datas;
  GenerateReq(8, req_comm_datas);
  std::vector<std::string> reqs;
  reqs.resize(req_comm_datas.size());
  for (int i = 0; i < req_comm_datas.size(); ++i) {
    auto req = req_comm_datas[i];
    for (int idx = 0; idx < req.keys_size(); ++idx) {
      auto* value_list = req.add_vals();
      int field_id = req.fields(idx);
      for (int j = 0; j < model_weight_schema[field_id]->total_weight_size(); ++j) {
        float g = UniformRandom(-0.1, 0.1);
        value_list->add_val(g);
      }
    }
    req.SerializeToString(&reqs[i]);
  }
  std::string req_id("req_id_1000");
  client->Push(gdef.name(), reqs, value_type, 10, req_id);
  LOG(INFO) << __FUNCTION__ << " done.";
}

void PsHandler_PushPull_Loop(PsClientPtr& client, proto::GraphDef& gdef, const std::string value_type, SchemaType model_weight_schema, int count) {
  for (int i = 0; i < count; ++i) {
    PsHandler_Push(client, gdef, value_type, model_weight_schema);
    PsHandler_Pull(client, gdef);
  }
}

void PsHandler_Dump(PsClientPtr& client, proto::GraphDef& gdef) {
  std::string val_type("WEIGHT");
  //std::string val_type("CHECKPOINT");
  client->Dump(gdef.name(), dump_and_load_path, val_type, "format", false);
}

void PsHandler_Load(PsClientPtr& client, proto::GraphDef& gdef) {
  client->Load(gdef.name(), dump_and_load_path);
}

void PsHandler_Drop(PsClientPtr& client, proto::GraphDef& gdef) {
  client->Drop(gdef.name());
}

void PsHandler_Move(PsClientPtr& client, proto::GraphDef& gdef) {
  std::string to = gdef.name();
  std::string from = to + "2";
  gdef.set_name(from);
  PsHandler_Create(client, gdef);

  Executor exec(gdef);
    if (exec.Init()) {
      LOG(ERROR) << "executor init failed.";
      return;
    }
    SchemaType model_weight_schema;
    ModelParser::CreateModelWeightSchema(exec.GetGraph(), model_weight_schema);
    LOG(INFO) << __FUNCTION__ << " model weight schema size: " << model_weight_schema.size();

  PsHandler_PushPull_Loop(client, gdef, "GRADIENT", model_weight_schema, 10);
  
  PsHandler_Dump(client, gdef);
  
  std::string backup = to + ".tmp";
  client->Move(from, to, backup);
}

void PsHandler_ListModels(PsClientPtr& client, proto::GraphDef& gdef) {
  std::vector<std::string> models;
  client->ListModels(models);
  LOG(INFO) << __FUNCTION__ << " model size:" << models.size();
  for (int i = 0; i < models.size(); ++i) {
    LOG(INFO) << "i: " << i << ", model:" << models[i];
  }
}

int main(int argc, char** argv) {
  init_log(argv[0], "./log");

  gflags::ParseCommandLineFlags(&argc, &argv, true);
  
  LOG(INFO) << "FLAGS_hosts: " << FLAGS_hosts;
  LOG(INFO) << "FLAGS_ports: " << FLAGS_ports;
  std::vector<std::string> hosts;
  boost::split(hosts, FLAGS_hosts, boost::is_any_of(","));
  std::vector<std::string> ports_str;
  boost::split(ports_str, FLAGS_ports, boost::is_any_of(",")); 
  std::vector<int> ports;
  for (auto& port: ports_str) { 
    ports.emplace_back(boost::lexical_cast<int>(port));
  }
  
  std::shared_ptr<ThriftClientWrapper<PsClient> > thrift_client_wrapper;
  thrift_client_wrapper.reset(new ThriftClientWrapper<PsClient>(hosts, ports, FLAGS_conn_timeout, FLAGS_timeout));
  std::shared_ptr<PsClient> client = thrift_client_wrapper->GetThriftClient();
  
  try {
    client->getInputProtocol()->getTransport()->open();
    std::string mode;
    client->Mode(mode);
    LOG(INFO) << "Mode: " << mode;
    int shard_num = client->ShardNum();
    LOG(INFO) << "ps shard_num: " << shard_num;
    std::string serving_type;
    client->ServingType(serving_type);
    LOG(INFO) << "ps serving_type: " << serving_type;

    proto::GraphDef gdef;
    if (ProtobufOp::LoadObjectFromPbFile<proto::GraphDef>(graph_file, &gdef) != 0) {
      LOG(ERROR) << "load graph def proto file failed.";
      return -1;
    }

    PsHandler_Create(client, gdef);
    
    // PsHandler_ModelDef(client, gdef);

    Executor exec(gdef);
    if (exec.Init()) {
      LOG(ERROR) << "executor init failed.";
      return -1;
    }
    SchemaType model_weight_schema;
    ModelParser::CreateModelWeightSchema(exec.GetGraph(), model_weight_schema);
    LOG(INFO) << "model weight schema size: " << model_weight_schema.size();

    PsHandler_Pull(client, gdef);

    int loop = 5;
    PsHandler_PushPull_Loop(client, gdef, "GRADIENT", model_weight_schema, loop);

    PsHandler_Dump(client, gdef);
    PsHandler_Load(client, gdef);
    PsHandler_Drop(client, gdef);

    PsHandler_Move(client, gdef);

    PsHandler_ListModels(client, gdef);
  } catch (TException& tx) {
    LOG(ERROR) << "TException " << tx.what();
  }

  gflags::ShutDownCommandLineFlags();
  return 0;
}
