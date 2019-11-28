/*!
 *  Copyright 2017 by Contributors
 *  \file memory_pool.h
 *  \brief the completion of memory pool
 *  \author ZhouYong
 */
#ifndef OPENMI_PS_INTERNAL_MEMORY_POOL_H_
#define OPENMI_PS_INTERNAL_MEMORY_POOL_H_ 

#include <memory>
#include <string>
#include <vector>
#include "base/logging.h"
using namespace openmi;

namespace openmi {
namespace ps {

const size_t BLOCK_SIZE = 2*1024*1024; // 2MB
const size_t POOL_ALIGN = 8;    // 申请资源（size）以POOL_ALIGN个字符数为单位

class MemoryPool {
public:
  MemoryPool() {}

  inline size_t AlignedSize(size_t size) {
    return ((size - 1) / POOL_ALIGN + 1) * POOL_ALIGN;
  }

  inline std::string& AddBlock() {
    blocks_.resize(blocks_.size() + 1);
    auto& block = blocks_.back();
    block.reserve(BLOCK_SIZE);   // capacity
    return block;
  }

  inline char* Allocate(size_t& size) {
    size = AlignedSize(size);
    if (blocks_.empty()) AddBlock();

    auto& block = blocks_.back();
    auto cur_size = block.size();
    if (cur_size + size <= block.capacity()) {
      block.resize(cur_size + size);
      return (char*)(block.data() + cur_size);
    } else {
      auto& block = AddBlock();
      block.resize(size);
      return (char*)(block.data());
    }
  }

  void Stat(size_t& mem_size, size_t& mem_capacity) {
    for (auto& b: blocks_) {
      mem_size += b.size();
      mem_capacity += b.capacity();
    }
  }

private:
  std::vector<std::string> blocks_;
}; // class MemoryPool

typedef std::shared_ptr<MemoryPool> MemoryPoolPtr;

} // namespace ps
} // namespace openmi
#endif // OPENMI_PS_INTERNAL_MEMORY_POOL_H_
