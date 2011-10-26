#ifndef IV_BLOOM_FILTER_H_
#define IV_BLOOM_FILTER_H_
#include "detail/functional.h"
namespace iv {
namespace core {

template<typename Integer>
class BloomFilter {
 public:
  typedef Integer value_type;
  BloomFilter() : filter_(0) { }

  void Add(value_type val) {
    filter_ |= val;
  }

  bool Contains(value_type val) const {
    return ((filter_ & val) == val);
  }

 private:
  value_type filter_;
};

template<typename T>
class HashedBloomFilter : private BloomFilter<std::size_t> {
 public:
  typedef BloomFilter<std::size_t> super_type;

  void Add(const T& val) {
    super_type::Add(std::hash<T>()(val));
  }

  bool Contains(const T& val) const {
    return super_type::Contains(std::hash<T>()(val));
  }
};

} }  // namespace iv::core
#endif  // IV_BLOOM_FILTER_H_
