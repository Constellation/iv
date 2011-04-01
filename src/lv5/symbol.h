#ifndef _IV_LV5_SYMBOL_H_
#define _IV_LV5_SYMBOL_H_
#include <cstddef>
#include <utility>
#include <tr1/type_traits>
#include <gc/gc_allocator.h>
#include "static_assert.h"
namespace iv {
namespace lv5 {

struct Symbol {
  std::size_t value_as_index;

  static Symbol FromSymbol(std::size_t value) {
    Symbol sym = { value };
    return sym;
  }
};

inline bool operator==(const Symbol& x, const Symbol& y) {
  return x.value_as_index == y.value_as_index;
}

inline bool operator!=(const Symbol& x, const Symbol& y) {
  return x.value_as_index != y.value_as_index;
}

inline bool operator<(const Symbol& x, const Symbol& y) {
  return x.value_as_index < y.value_as_index;
}

inline bool operator>(const Symbol& x, const Symbol& y) {
  return x.value_as_index > y.value_as_index;
}

inline bool operator<=(const Symbol& x, const Symbol& y) {
  return x.value_as_index <= y.value_as_index;
}

inline bool operator>=(const Symbol& x, const Symbol& y) {
  return x.value_as_index <= y.value_as_index;
}

#if defined(__GNUC__) && (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 3)
IV_STATIC_ASSERT(std::tr1::is_pod<Symbol>::value);
#endif

} }  // namespace iv::lv5

GC_DECLARE_PTRFREE(iv::lv5::Symbol);

namespace std {
namespace tr1 {
// template specialization for Symbol in std::tr1::unordered_map
template<>
struct hash<iv::lv5::Symbol>
  : public unary_function<iv::lv5::Symbol, std::size_t> {
  inline result_type operator()(const argument_type& x) const {
    return hash<std::size_t>()(x.value_as_index);
  }
};
} }  // namespace std::tr1
#endif  // _IV_LV5_SYMBOL_H_
