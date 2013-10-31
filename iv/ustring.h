#ifndef IV_USTRING_H_
#define IV_USTRING_H_
#include <string>
#include <iv/detail/functional.h>
#include <iv/detail/cstdint.h>
#include <iv/stringpiece.h>
#include <iv/conversions.h>
namespace iv {
namespace core {

typedef std::u16string UString;

inline UString ToUString(const StringPiece& piece) {
  return UString(piece.begin(), piece.end());
}

inline UString ToUString(const UStringPiece& piece) {
  return UString(piece.begin(), piece.end());
}

inline UString ToUString(char16_t ch) {
  return UString(&ch, &ch + 1);
}

} }  // namespace iv::core
#endif  // IV_USTRING_H_
