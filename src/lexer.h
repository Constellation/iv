#ifndef _IV_LEXER_H_
#define _IV_LEXER_H_

#include <cstdlib>
#include <cctype>
#include <vector>
#include <string>
#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include "char.h"
#include "token.h"
#include "noncopyable.h"

namespace iv {
namespace core {

class Lexer: private Noncopyable {
 public:
  enum GetterOrSetter {
    kNotGetterOrSetter = 0,
    kGetter,
    kSetter
  };
  explicit Lexer(const std::string&);
  explicit Lexer(const char*);
  Token::Type Next();
  inline Token::Type Peek() { return current_token_; }
  inline const UChar* Literal() const {
    return buffer16_.data();
  }
  inline const char* Literal8() const {
    return buffer8_.data();
  }
  inline const double& Numeric() const {
    return numeric_;
  }
  inline GetterOrSetter IsGetterOrSetter() const {
    return getter_or_setter_;
  }
  inline bool has_line_terminator_before_next() const {
    return has_line_terminator_before_next_;
  }
  uint32_t line_number() const {
    return line_number_;
  }
  uint32_t pos() const {
    return pos_;
  }
  bool ScanRegExpLiteral(bool contains_eq);
  bool ScanRegExpFlags();

 private:
  static const std::size_t kInitialReadBufferCapacity = 32;
  enum NumberType {
    DECIMAL,
    HEX,
    OCTAL
  };

  inline void Advance() {
    if (pos_ == end_) {
      c_ = -1;
    } else {
      c_ = source_[pos_++];
    }
  }
  inline void Record8() {
    buffer8_.push_back(static_cast<char>(c_));
  }
  inline void Record8(const int ch) {
    buffer8_.push_back(static_cast<char>(ch));
  }
  inline void Record16() { buffer16_.push_back(c_); }
  inline void Record16(const int ch) { buffer16_.push_back(ch); }
  inline void Record8Advance() {
    Record8();
    Advance();
  }
  inline void Record16Advance() {
    Record16();
    Advance();
  }
  void PushBack();
  inline Token::Type IsMatch(char const * keyword,
                    std::size_t len,
                    Token::Type guess) const;
  Token::Type SkipSingleLineComment();
  Token::Type SkipMultiLineComment();
  Token::Type ScanHtmlComment();
  Token::Type ScanIdentifier();
  Token::Type DetectKeyword();
  Token::Type ScanString();
  void ScanEscape();
  Token::Type ScanNumber(const bool period);
  UChar ScanOctalEscape();
  UChar ScanHexEscape(UChar c, int len);
  inline int OctalValue(const int c) const;
  inline int HexValue(const int c) const;
  void ScanDecimalDigits();
  inline void SkipLineTerminator();

  const UnicodeString source_;
  std::vector<char> buffer8_;
  std::vector<UChar> buffer16_;
  double numeric_;
  uint32_t pos_;
  const uint32_t end_;
  bool has_line_terminator_before_next_;
  int c_;
  uint32_t line_number_;
  GetterOrSetter getter_or_setter_;
  Token::Type current_token_;
};


} }  // namespace iv::core

#endif  // _IV_LEXER_H_

