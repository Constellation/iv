#ifndef _IV_LEXER_H_
#define _IV_LEXER_H_

#include <cstdlib>
#include <cassert>
#include <vector>
#include <string>
#include <iostream>
#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include "char.h"
#include "token.h"
#include "source.h"
#include "noncopyable.h"

namespace iv {
namespace core {

class Lexer: private Noncopyable<Lexer>::type {
 public:
  enum LexType {
    kIdentifyReservedWords = 0,
    kIgnoreReservedWords,
    kIgnoreReservedWordsAndIdentifyGetterOrSetter
  };
  enum State {
    NONE,
    ESCAPE,
    DECIMAL,
    HEX,
    OCTAL
  };

  explicit Lexer(Source* src);
  Token::Type Next(LexType type);
  inline const std::vector<UChar>& Buffer() const {
    return buffer16_;
  }
  inline const std::vector<char>& Buffer8() const {
    return buffer8_;
  }
  inline const double& Numeric() const {
    return numeric_;
  }
  inline State NumericType() const {
    assert(type_ == DECIMAL ||
           type_ == HEX ||
           type_ == OCTAL);
    return type_;
  }
  inline State StringEscapeType() const {
    assert(type_ == NONE ||
           type_ == ESCAPE ||
           type_ == OCTAL);
    return type_;
  }
  inline bool has_line_terminator_before_next() const {
    return has_line_terminator_before_next_;
  }
  std::size_t line_number() const {
    return line_number_;
  }
  std::size_t pos() const {
    return pos_;
  }
  inline Source* source() const {
    return source_;
  }
  bool ScanRegExpLiteral(bool contains_eq);
  bool ScanRegExpFlags();

 private:
  static const std::size_t kInitialReadBufferCapacity = 32;
  void Initialize();
  inline void Advance() {
    if (pos_ == end_) {
      c_ = -1;
    } else {
      c_ = source_->Get(pos_++);
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
  Token::Type ScanMagicComment();
  Token::Type ScanIdentifier(LexType type);
  Token::Type DetectKeyword() const;
  Token::Type DetectGetOrSet() const;
  Token::Type ScanString();
  void ScanEscape();
  Token::Type ScanNumber(const bool period);
  UChar ScanOctalEscape();
  UChar ScanHexEscape(UChar c, int len);
  inline int OctalValue(const int c) const;
  inline int HexValue(const int c) const;
  void ScanDecimalDigits();
  inline void SkipLineTerminator();

  Source* source_;
  std::vector<char> buffer8_;
  std::vector<UChar> buffer16_;
  double numeric_;
  State type_;
  std::size_t pos_;
  const std::size_t end_;
  bool has_line_terminator_before_next_;
  bool has_shebang_;
  int c_;
  std::size_t line_number_;
};


} }  // namespace iv::core

#endif  // _IV_LEXER_H_
