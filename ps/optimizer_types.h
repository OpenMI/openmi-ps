#ifndef OPENMI_CORE_ENGINE_OPTIMIZER_OPTIMIZER_TYPES_H_
#define OPENMI_CORE_ENGINE_OPTIMIZER_OPTIMIZER_TYPES_H_

#include "openmi/idl/proto/optimizer.pb.h"
#include "ftrl.h"
#include "openmi/core/framework/types.h"
using namespace openmi;
using namespace openmi::proto;

namespace openmi {


#define OPENMI_OPTIMIZER_CASE(optimizer_param_type, ARGS) \
  typedef optimizer_param_type T; \
  ARGS; \
  break;

#define OPENMI_OPTIMIZER_CASES(optimizer_type, ARGS) \
  switch (optimizer_type) {   \
    case ADAGRAD: { \
      OPENMI_OPTIMIZER_CASE(AdaGradParam, DIRECT_ARG(ARGS)) \
    } \
    case FTRL: { \
      OPENMI_OPTIMIZER_CASE(FtrlParam, DIRECT_ARG(ARGS)) \
    } \
    default: printf("OPENMI_OPTIMIZER_CASES error");  \
  } 

inline size_t SizeOfOptimizerType(OptimizerType type) {
  size_t ret = 0;
  OPENMI_OPTIMIZER_CASES(type, ret = sizeof(T));
  return ret;
}

}; 
#endif // OPENMI_CORE_ENGINE_OPTIMIZER_OPTIMIZER_TYPES_H_