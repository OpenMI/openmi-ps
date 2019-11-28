#include <string>
#include <vector>

#include "base/logging.h"
#include "openmi/core/distribute_runtime/thrift_client_wrapper.h"
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

void init_log(const char* name, const char* log_dir) {
  InitLogging(name, log_dir);
  g_alsologtostderr = false;
  g_log_severity = INFO;
  LOG(INFO) << "init logging done. name: " << name << ", logging dir: " << log_dir << ", g_log_dir: " << g_log_dir;
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
  } catch (TException& tx) {
    LOG(ERROR) << "TException " << tx.what();
  }

  gflags::ShutDownCommandLineFlags();
}
