#ifndef IV_USTRING_H_
#define IV_USTRING_H_
#include <string>
#include <iv/detail/functional.h>
#include <iv/detail/cstdint.h>
#include <iv/string_view.h>
#include <iv/conversions.h>
#include <iv/unicode_character.h>
namespace iv {
namespace core {

inline std::u16string ToU16String(const string_view& piece) {
  return std::u16string(
      reinterpret_cast<const uc8*>(piece.data()),
      reinterpret_cast<const uc8*>(piece.data() + piece.size()));
}

inline std::u16string ToU16String(const u16string_view& piece) {
  return std::u16string(piece.begin(), piece.end());
}

inline std::u16string ToU16String(char16_t ch) {
  return std::u16string(&ch, &ch + 1);
}

} }  // namespace iv::core
#endif  // IV_USTRING_H_
