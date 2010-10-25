#ifndef _IV_CHARS_H_
#define _IV_CHARS_H_
#include <tr1/cstdint>
#include "uchar.h"

namespace iv {
namespace core {

class Chars {
 public:
  enum CharCategory {
    UPPERCASE_LETTER = U_MASK(U_UPPERCASE_LETTER),
    LOWERCASE_LETTER = U_MASK(U_LOWERCASE_LETTER),
    TITLECASE_LETTER = U_MASK(U_TITLECASE_LETTER),
    MODIFIER_LETTER  = U_MASK(U_MODIFIER_LETTER),
    OTHER_LETTER     = U_MASK(U_OTHER_LETTER),
    NON_SPACING_MARK = U_MASK(U_NON_SPACING_MARK),
    COMBINING_SPACING_MARK = U_MASK(U_COMBINING_SPACING_MARK),
    DECIMAL_DIGIT_NUMBER   = U_MASK(U_DECIMAL_DIGIT_NUMBER),
    SPACE_SEPARATOR = U_MASK(U_SPACE_SEPARATOR),
    CONNECTOR_PUNCTUATION  = U_MASK(U_CONNECTOR_PUNCTUATION)
  };
  static inline uint32_t Category(const int c) {
    return U_MASK(u_charType(c));
  }
  static inline bool IsASCII(const int c) {
    return !(c & ~0x7F);
  }
  static inline bool IsASCIIAlpha(const int c) {
    return (c | 0x20) >= 'a' && (c | 0x20) <= 'z';
  }
  static inline bool IsASCIIAlphanumeric(const int c) {
    return (c >= '0' && c <= '9') || ((c | 0x20) >= 'a' && (c | 0x20) <= 'z');
  }
  static inline bool IsNonASCIIIdentifierStart(const int c) {
    return Category(c) & (UPPERCASE_LETTER | LOWERCASE_LETTER |
                          TITLECASE_LETTER | MODIFIER_LETTER  | OTHER_LETTER);
  }
  static inline bool IsNonASCIIIdentifierPart(const int c) {
    return Category(c) & (UPPERCASE_LETTER | LOWERCASE_LETTER |
                          TITLECASE_LETTER | MODIFIER_LETTER  |
                          OTHER_LETTER     | NON_SPACING_MARK |
                          COMBINING_SPACING_MARK |
                          DECIMAL_DIGIT_NUMBER   |
                          CONNECTOR_PUNCTUATION);
  }
  static inline bool IsSeparatorSpace(const int c) {
    return u_charType(c) == U_SPACE_SEPARATOR;
  }
  static inline bool IsWhiteSpace(const int c) {
    return IsASCII(c) ?
        (c == ' ' || c == '\t' || c == 0xB || c == 0xC) : IsSeparatorSpace(c);
  }
  static inline bool IsLineTerminator(const int c) {
    return c == '\r' || c == '\n' || (c & ~1) == 0x2028;
  }
  static inline bool IsHexDigit(const int c) {
    return (c >= '0' && c <= '9') || ((c | 0x20) >= 'a' && (c | 0x20) <= 'f');
  }
  static inline bool IsDecimalDigit(const int c) {
    return c >= '0' && c <= '9';
  }
  static inline bool IsOctalDigit(const int c) {
    return c >= '0' && c <= '7';
  }
  static inline bool IsIdentifierStart(const int c) {
    return IsASCII(c) ? c == '$'  || c == '_' ||
                        c == '\\' || IsASCIIAlpha(c) :
                        IsNonASCIIIdentifierStart(c);
  }
  static inline bool IsIdentifierPart(const int c) {
    return IsASCII(c) ? c == '$'  || c == '_' ||
                        c == '\\' || IsASCIIAlphanumeric(c) :
                        IsNonASCIIIdentifierPart(c);
  }
};

} }  // namespace iv::core

#endif  // _IV_CHARS_H_
