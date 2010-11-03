#ifndef _IV_CHARS_H_
#define _IV_CHARS_H_
#include <cassert>
#include <tr1/cstdint>
#include "uchar.h"
#include "ucdata.h"

namespace iv {
namespace core {

class Chars {
 public:
  enum CharCategory {
    UPPERCASE_LETTER = 1 << UC16::kUpperCaseLetter,
    LOWERCASE_LETTER = 1 << UC16::kLowerCaseLetter,
    TITLECASE_LETTER = 1 << UC16::kTitleCaseLetter,
    MODIFIER_LETTER  = 1 << UC16::kModifierLetter,
    OTHER_LETTER     = 1 << UC16::kOtherLetter,
    NON_SPACING_MARK = 1 << UC16::kNonSpacingMark,
    COMBINING_SPACING_MARK = 1 << UC16::kCombiningSpacingMark,
    DECIMAL_DIGIT_NUMBER   = 1 << UC16::kDecimalDigitNumber,
    SPACE_SEPARATOR = 1 << UC16::kSpaceSeparator,
    CONNECTOR_PUNCTUATION  = 1 << UC16::kConnectorPunctuation
  };
  static inline uint32_t Category(const int c) {
    assert(c <= 0xffff+1);
    return (c < 0) ? 1 : (1 << UnicodeData::kCategory[c]);
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
    return UnicodeData::kCategory[c] == UC16::kSpaceSeparator;
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
