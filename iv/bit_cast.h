#ifndef IV_BIT_CAST_H_
#define IV_BIT_CAST_H_
#include <iv/platform.h>
#include <iv/static_assert.h>
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
  IV_STATIC_ASSERT(sizeof(To) == sizeof(From));
  return BitCastNoGuard<To, From>(from);
}

} }  // namespace iv::core
#endif  // IV_BIT_CAST_H_
