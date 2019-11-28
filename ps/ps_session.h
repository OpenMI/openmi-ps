#ifndef OPENMI_PS_PS_SESSION_H_
#define OPENMI_PS_PS_SESSION_H_

#include "base/synchronizer.h"
#include "ps/model.h"
#include "openmi/idl/proto/model.pb.h"
using namespace openmi;

namespace openmi {
namespace ps {

class PsSession {
public:
  PsSession(const std::string& name): name_(name) {}
  ~PsSession() {}

  int Init(ModelPtr model) {
    model_ = model;
    return 0;
  }

  std::string Name() {
    return name_;
  }

  void Pull(std::vector<std::string>& _return, const std::vector<std::string>& reqs, const std::string& value_type, const std::string& req_id);
  void Push(const std::vector<std::string>& reqs, const std::string& value_type, const std::string& req_id);

  void Dump(const std::string& path, proto::ModelValueType value_type, const std::string& format, const bool dump_zero);
  void Load(const std::string& path, proto::ModelValueType value_type, const std::string& format);

private: 
  std::string name_;
  ModelPtr model_;
}; // class PsSession

typedef std::shared_ptr<PsSession> PsSessionPtr;

} // namespace ps
} // namespace openmi
#endif // OPENMI_PS_PS_SESSION_H_