#ifndef IV_UNICODE_CHARACTER_H_
#define IV_UNICODE_CHARACTER_H_
#include <iv/detail/cstdint.h>
#include <type_traits>
namespace iv {
namespace core {

// These types are ensured as unsigned.
typedef uint8_t uc8;
typedef char16_t uc16;

static_assert(!std::is_signed<char16_t>::value, "char16_t should be unsigned");

} }  // namespace iv::core
#endif  // IV_UNICODE_CHARACTER_H_
