#include "ps/internal/kv_storer.h"
#include "base/logging.h"
// #include "ps/ftrl.h"
using namespace openmi;

typedef float VALUE;

class BaseTest {
}; 

class FtrlOptimizer : public BaseTest {
public:

  FtrlOptimizer(): n_(0), v_(0), k_(0) {}
  ~FtrlOptimizer() {}

  int Size() {
    return sizeof(FtrlOptimizer);
  }

  short X() { return 1; }
 
  float n_;
  float v_;
  float k_;
};

class AdagradOptimizer {
public:
  AdagradOptimizer() {}
  ~AdagradOptimizer() {}

  float n_;
};

void NumericStoreTest(ps::KVStorer& kv);
void OptimizerStoretest(ps::KVStorer& kv);


int main(int argc, char** argv) {
  ps::KVStorer kv;
  size_t data_num = 100;
  size_t ptr_data_num = 100;
  kv.Init(data_num, ptr_data_num);

  LOG(INFO) << "sizeof(AdagradOptimizer): " << sizeof(AdagradOptimizer);
  LOG(INFO) << "sizeof(FtrlOptimizer): " << sizeof(FtrlOptimizer);

  OptimizerStoretest(kv);

  return 0;
}


void OptimizerStoretest(ps::KVStorer& kv) {
  FID fid = 10002L;
  bool created = false;
  int size1 = sizeof(FtrlOptimizer);
  int size2 = sizeof(AdagradOptimizer);
  FtrlOptimizer ftrlClass;
  LOG(INFO) << "size1: " << size1 << ", but ftrlClass.Size: " << ftrlClass.Size();
  LOG(INFO) << "sizeof(BaseTest): " << sizeof(BaseTest);
  void* data = kv.GetOrCreate(fid, size1 + size2, created);
  FtrlOptimizer* ftrl = reinterpret_cast<FtrlOptimizer*>(data);
  data = reinterpret_cast<char*>(data) + size1;
  AdagradOptimizer* adagrad = reinterpret_cast<AdagradOptimizer*>(data);
  ftrl->n_ = 0.1f;
  ftrl->v_ = 0.11f;
  ftrl->k_ = 0.111f;
  adagrad->n_ = 0.2f;

  void* data2 = kv.Get(fid, size1 + size2);
  FtrlOptimizer* ftrl1 = reinterpret_cast<FtrlOptimizer*>(data2);
  data2 = reinterpret_cast<char*>(data2) + size1;
  AdagradOptimizer* adagrad1 = reinterpret_cast<AdagradOptimizer*>(data2);
  LOG(INFO) << " ftrl1.n:" << ftrl1->n_ << ", adagrad1.n:" << adagrad1->n_;
}

void NumericStoreTest(ps::KVStorer& kv) {
   FID fid1 = 10000l;
  bool created = false;
  auto size = sizeof(VALUE) * 3;
  void* data = kv.GetOrCreate(fid1, size, created);

  //void* data = kv.Get(fid1, size);
  VALUE* fdata = reinterpret_cast<VALUE*>(data);
  *fdata = 0.1111;
  *(fdata+1) = 0.2222;
  LOG(INFO) << "before value. fdata1:" << *fdata << ", fdata2:" << *(fdata+1);

  *fdata = 0.1;
  LOG(INFO) << "*fdata:" << *fdata;
  LOG(INFO) << "*fdata:" << *fdata;
  *(fdata+1) = 0.2;
  *(fdata+2) = 0.3;
  LOG(INFO) << "after value. fdata1:" << *fdata << ", fdata2:" << *(fdata+1) << ", test:" << *(fdata+2);

  void* data2 = kv.GetOrCreate(fid1, size, created);
  VALUE* fdata2 = reinterpret_cast<VALUE*>(data2);
  LOG(INFO) << "again get value. fdata1:" << *fdata2 << ", fdata2:" << *(fdata2+1) << ", fdata3:" << *(fdata2+2) << ", fdata4:" << *(fdata2+3);

  void* x = new char[8]();
  float* fx = reinterpret_cast<float*>(x);
  *fx = 0.11;
  *(fx+1) = 0.22;
  float* fx2 = reinterpret_cast<float*>(x);
  LOG(INFO) << "x[0]:" << *fx2 << ", x[1]:" << *(fx2+1);
}