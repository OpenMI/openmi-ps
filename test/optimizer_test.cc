#include "openmi/idl/proto/optimizer.pb.h"
#include "openmi/idl/proto/engine.pb.h"
#include "openmi/idl/proto/communication.pb.h"
#include "ps/optimizer_types.h"
#include "base/logging.h"
using namespace openmi;

int main(int argc, char** argv) {
  int ftrl_size = SizeOfOptimizerType(proto::FTRL);
  int adagrad_size = SizeOfOptimizerType(proto::ADAGRAD);
  LOG(INFO) << "ftrl size: " << ftrl_size << ", adagrad size: " << adagrad_size;
  return 0;
}
