#ifndef IV_BIT_CAST_H_
#define IV_BIT_CAST_H_
#include <iv/static_assert.h>
namespace iv {
namespace core {

template<typename To, typename From>
inline To BitCast(From from) {
  IV_STATIC_ASSERT(sizeof(To) == sizeof(From));
  union {
    From from_;
    To to_;
  } value;
  value.from_ = from;
  return value.to_;
}

} }  // namespace iv::core
#endif  // IV_BIT_CAST_H_
