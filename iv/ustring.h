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

typedef std::u16string UString;

inline UString ToUString(const string_view& piece) {
  return UString(reinterpret_cast<const uc8*>(piece.data()),
                 reinterpret_cast<const uc8*>(piece.data() + piece.size()));
}

inline UString ToUString(const u16string_view& piece) {
  return UString(piece.begin(), piece.end());
}

inline UString ToUString(char16_t ch) {
  return UString(&ch, &ch + 1);
}

} }  // namespace iv::core
#endif  // IV_USTRING_H_
