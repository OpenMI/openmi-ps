#ifndef OPENMI_PS_INTERNAL_KV_STORER_H_
#define OPENMI_PS_INTERNAL_KV_STORER_H_

#include <string.h>
#include <sparsehash/sparse_hash_map>
#include "ps/internal/hash_city.h"
#include "ps/internal/memory_pool.h"
#include "base/logging.h"
using namespace openmi::ps;

namespace openmi {
namespace ps {

const size_t MAP_ALIGN = 8;
typedef uint64_t FID;

struct RawData;

struct RawData {
  char data[MAP_ALIGN];
  int bytes;
  RawData(): bytes(0) {}
};

struct PtrData {
  char* data = nullptr;
  int bytes;
  PtrData(): bytes(0) {}
};

using DataHandler = std::function<void(FID key, void* data)>;
using KVStorerHandler = std::function<void(FID key, void* data, int size)>;

class KVStorer {
public:
  KVStorer() {
    raw_kv_.set_deleted_key(-1);
    ptr_kv_.set_deleted_key(-1);
  }

  // data_num为key的个数
  inline void Init(size_t data_num, size_t ptr_data_num) {
    raw_kv_.resize(data_num);
    ptr_kv_.resize(ptr_data_num);
  }

  inline char* Get(FID key, size_t size) {
    if (size <= MAP_ALIGN) {
      auto it = raw_kv_.find(key);
      if (it != raw_kv_.end()) {
        return it->second.data;
      }
    } else {
      auto it = ptr_kv_.find(key);
      if (it != ptr_kv_.end()) {
        return it->second.data;
      }
    }
    return nullptr;
  }

  inline char* GetOrCreate(FID key, size_t size, bool& created) {
    if (size <= MAP_ALIGN) {
      auto res = raw_kv_.insert(std::make_pair(key, RawData()));
      created = res.second;
      return res.first->second.data;
    } else {
      auto res = ptr_kv_.insert(std::make_pair(key, PtrData()));
      created = res.second;
      if (created) {
        res.first->second.data = memory_pool_.Allocate(size);
        res.first->second.bytes = size;
      }
      return res.first->second.data;
    }
  }

  inline void Update(FID key, void* data, size_t size) {
    bool created = false;
    auto* allocated_region = GetOrCreate(key, size, created);
    if (created) {
      memcpy(allocated_region, data, size);
    }
  }

  inline void ForEach(DataHandler& fn) {
    for (auto& it: raw_kv_) {
      fn(it.first, it.second.data);
    }

    for (auto& it: ptr_kv_) {
      fn(it.first, it.second.data);
    }
  }

  inline void ForEach(KVStorerHandler& fn) {
    for (auto& it: raw_kv_) {
      fn(it.first, it.second.data, it.second.bytes);
    }

    for (auto& it: ptr_kv_) {
      fn(it.first, it.second.data, it.second.bytes);
    }
  }

  inline void Stat(size_t& data_num, size_t& ptr_data_num, size_t& mem_size, size_t& mem_capacity) {
    data_num = raw_kv_.size();
    ptr_data_num = ptr_kv_.size();
    memory_pool_.Stat(mem_size, mem_capacity);
  }

private:
  MemoryPool memory_pool_;
  google::sparse_hash_map<FID, RawData, CityHash> raw_kv_;
  google::sparse_hash_map<FID, PtrData, CityHash> ptr_kv_;
}; // class KVStorer

typedef std::shared_ptr<KVStorer> KVStorerPtr;

} // namespace ps
} // namespace openmi
#endif // OPENMI_PS_INTERNAL_KV_STORER_H_
