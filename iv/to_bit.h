#ifndef IV_TO_BIT_H_
#define IV_TO_BIT_H_
#include <iv/platform.h>
#include <iv/detail/cstdint.h>
#include <iv/bit_cast.h>
namespace iv {
namespace core {
namespace to_bit_detail {

template<std::size_t N>
struct Rep;

template<>
struct Rep<1> {
  typedef uint8_t type;
};

template<>
struct Rep<2> {
  typedef uint16_t type;
};

template<>
struct Rep<4> {
  typedef uint32_t type;
};

template<>
struct Rep<8> {
  typedef uint64_t type;
};

}  // namespace to_bit_detail

template<typename To, typename From>
IV_ALWAYS_INLINE To ToBit(From val) {
  typedef typename to_bit_detail::Rep<sizeof(From)>::type bit_type;
#if defined(IV_COMPILER_GCC) && (IV_COMPILER_GCC > 40300)
  IV_STATIC_ASSERT(std::is_integral<To>::value);
#endif
  return static_cast<To>(core::BitCast<bit_type>(val));
}

} }  // namespace iv::core
#endif  // IV_TO_BIT_H_
