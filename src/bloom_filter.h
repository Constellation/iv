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

  value_type value() const {
    return filter_;
  }

 private:
  value_type filter_;
};

template<typename T, typename R = std::size_t>
class HashedBloomFilter : private BloomFilter<R> {
 public:
  typedef BloomFilter<R> super_type;

  void Add(const T& val) {
    super_type::Add(std::hash<T>()(val));
  }

  bool Contains(const T& val) const {
    return super_type::Contains(std::hash<T>()(val));
  }

  using super_type::value;
};

} }  // namespace iv::core
#endif  // IV_BLOOM_FILTER_H_
