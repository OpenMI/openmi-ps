#ifndef OPENMI_PS_INTERNAL_HASH_CITY_H_
#define OPENMI_PS_INTERNAL_HASH_CITY_H_

#include <city.h>

namespace openmi {
namespace ps {

inline uint64_t city_hash(const uint64_t id) {
  return Hash128to64(uint128(0x9ae16a3b2f90404fULL, id));
}

inline uint64_t city_hash_pair(const uint64_t id1, const uint64_t id2) {
  return Hash128to64(uint128(id1, id2));
}

class CityHash {
public:
  inline uint64_t operator()(const uint64_t id) const {
    return city_hash(id);
  }

  inline uint64_t operator()(const uint64_t id1, const uint64_t id2) const {
    return city_hash_pair(id1, id2);
  }
}; // class CityHash

} // namespace ps
} // namespace openmi
#endif // OPENMI_PS_INTERNAL_HASH_CITY_H_
