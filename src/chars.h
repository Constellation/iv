#ifndef _IV_CHARS_H_
#define _IV_CHARS_H_
#include <cassert>
#include <tr1/cstdint>
#include "uchar.h"
#include "character.h"

namespace iv {
namespace core {

class Chars {
 public:
  static inline uint32_t Category(const int c) {
    return (c < 0) ? 0 : character::GetCategory(c);
  }
  static inline bool IsASCII(const int c) {
    return (c >= 0) && character::IsASCII(c);
  }
  static inline bool IsASCIIAlpha(const int c) {
    return (c >= 0) && character::IsASCIIAlpha(c);
  }
  static inline bool IsASCIIAlphanumeric(const int c) {
    return (c >= 0) && character::IsASCIIAlphanumeric(c);
  }
  static inline bool IsNonASCIIIdentifierStart(const int c) {
    return (c >= 0) && character::IsNonASCIIIdentifierStart(c);
  }
  static inline bool IsNonASCIIIdentifierPart(const int c) {
    return (c >= 0) && character::IsNonASCIIIdentifierPart(c);
  }
  static inline bool IsSeparatorSpace(const int c) {
    return (c >= 0) && character::IsSeparatorSpace(c);
  }
  static inline bool IsWhiteSpace(const int c) {
    return (c >= 0) && character::IsWhiteSpace(c);
  }
  static inline bool IsLineTerminator(const int c) {
    return (c >= 0) && character::IsLineTerminator(c);
  }
  static inline bool IsHexDigit(const int c) {
    return (c >= 0) && character::IsHexDigit(c);
  }
  static inline bool IsDecimalDigit(const int c) {
    return (c >= 0) && character::IsDecimalDigit(c);
  }
  static inline bool IsOctalDigit(const int c) {
    return (c >= 0) && character::IsOctalDigit(c);
  }
  static inline bool IsIdentifierStart(const int c) {
    return (c >= 0) && character::IsIdentifierStart(c);
  }
  static inline bool IsIdentifierPart(const int c) {
    return (c >= 0) && character::IsIdentifierPart(c);
  }
};

} }  // namespace iv::core

#endif  // _IV_CHARS_H_
