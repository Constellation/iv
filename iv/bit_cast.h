#ifndef IV_BIT_CAST_H_
#define IV_BIT_CAST_H_
#include <iv/platform.h>
namespace iv {
namespace core {

template<typename To, typename From>
IV_ALWAYS_INLINE To BitCastNoGuard(From from) {
  union {
    From from_;
    To to_;
  } value;
  value.from_ = from;
  return value.to_;
}

template<typename To, typename From>
IV_ALWAYS_INLINE To BitCast(From from) {
  static_assert(sizeof(To) == sizeof(From), "To and From size should be equal");
  return BitCastNoGuard<To, From>(from);
}

} }  // namespace iv::core
#endif  // IV_BIT_CAST_H_
