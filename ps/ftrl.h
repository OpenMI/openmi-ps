#ifndef OPENMI_CORE_ENGINE_OPTIMZIER_FTRL_H_
#define OPENMI_CORE_ENGINE_OPTIMZIER_FTRL_H_

#include <cmath>
#include "optimizer.h"
#include "base/logging.h"
using namespace openmi;

namespace openmi {

struct AdaGradParam {
  AdaGradParam(): n_(0) {}
  float n_;
}; // struct FtrlParam


struct FtrlParam {
  FtrlParam(): z_(0), n_(0) {}
  float z_;
  float n_;
}; // struct FtrlParam

class FtrlOptimizer : public BaseOptimizer {
public: 
  FtrlOptimizer() {}
  ~FtrlOptimizer() {}

  int Compute(float* w, float g, char* opti_data, int& opti_bytes_offset, proto::OptimizerConfig& conf) {
    auto* opti_param = reinterpret_cast<FtrlParam*>(opti_data + opti_bytes_offset);
    auto z_ = opti_param->z_;
    auto n_ = opti_param->n_;
    auto w_ = *w;

    LOG(INFO) << "z_:" << z_ << ", n_:" << n_ << ", w:" << *w << ", g:" << g;

    auto accum_square = n_ + g*g;
    auto sigma = (std::sqrt(accum_square) - std::sqrt(n_)) / conf.alpha();
    z_ += g - sigma * (w_);

    if (std::isinf(z_)) {
      LOG(ERROR) << "illegal weight update. occur 'inf'. g:" << g << ", n_:" << n_ << ", z_:" << z_ << ", accum_square:" << accum_square;
      return -1;
    }

    if (std::abs(z_) < conf.lambda1()) {
      w_ = 0;
    } else {
      auto sign = z_ > 0 ? 1.0f : -1.0f;
      auto x = (sign * conf.lambda1() - z_);
      auto y = (conf.beta() + std::sqrt(n_)) / conf.alpha() + conf.lambda2();
      w_ = x / y;
    }

    opti_param->n_ = accum_square;
    opti_param->z_ = z_;
    *w = w_;

    LOG(INFO) << "updated z_:" << z_ << ", n_:" << n_ << ", w:" << *w;

    opti_bytes_offset += sizeof(FtrlParam);
    return 0;
  }

  int Size() {
    return sizeof(FtrlOptimizer);
  }
}; // class FtrlOptimizer

} // namespace openmi
#endif // OPENMI_CORE_ENGINE_OPTIMZIER_FTRL_H_