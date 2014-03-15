#ifndef IV_STRING_H_
#define IV_STRING_H_
#include <string>
#include <cstdint>
#include <iv/string_view.h>
namespace iv {
namespace core {

inline std::u16string ToU16String(const string_view& piece) {
  // TODO(Yusuke Suzuki): This cast is not necessary.
  return std::u16string(
      reinterpret_cast<const char16_t*>(piece.data()),
      reinterpret_cast<const char16_t*>(piece.data() + piece.size()));
}

inline std::u16string ToU16String(const u16string_view& piece) {
  return std::u16string(piece.begin(), piece.end());
}

inline std::u16string ToU16String(char16_t ch) {
  return std::u16string(&ch, &ch + 1);
}

} }  // namespace iv::core
#endif  // IV_STRING_H_
