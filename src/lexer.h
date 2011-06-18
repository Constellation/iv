#ifndef _IV_LEXER_H_
#define _IV_LEXER_H_
#include <cstddef>
#include <cassert>
#include <cstdlib>
#include <vector>
#include <string>
#include "detail/cstdint.h"
#include "character.h"
#include "token.h"
#include "location.h"
#include "noncopyable.h"
#include "keyword.h"
#include "conversions.h"
#include "source_traits.h"

namespace iv {
namespace core {

template<typename Source, bool RecognizeCommentAsToken = false>
class Lexer: private Noncopyable<> {
 public:

  enum State {
    NONE,
    ESCAPE,
    DECIMAL,
    HEX,
    OCTAL
  };

  explicit Lexer(const Source& src)
      : source_(src),
        buffer8_(),
        buffer16_(kInitialReadBufferCapacity),
        pos_(0),
        end_(source_.size()),
        has_line_terminator_before_next_(false),
        has_shebang_(false),
        line_number_(1),
        previous_location_(),
        location_() {
    Advance();
  }

  template<typename LexType>
  typename Token::Type Next(bool strict) {
    typename Token::Type token;
    has_line_terminator_before_next_ = false;
    StorePreviousLocation();
    do {
      while (c_ >= 0 && character::IsWhiteSpace(c_)) {
        // white space
        Advance();
      }
      location_.set_begin_position(pos() - 1);
      switch (c_) {
        case '"':
        case '\'':
          // string literal
          token = ScanString();
          break;

        case '<':
          // < <= << <<= <!--
          Advance();
          if (c_ == '=') {
            Advance();
            token = Token::TK_LTE;
          } else if (c_ == '<') {
            Advance();
            if (c_ == '=') {
              Advance();
              token = Token::TK_ASSIGN_SHL;
            } else {
              token = Token::TK_SHL;
            }
          } else if (c_ == '!') {
            token = SkipHtmlComment();
          } else {
            token = Token::TK_LT;
          }
          break;

        case '>':
          // > >= >> >>= >>> >>>=
          Advance();
          if (c_ == '=') {
            Advance();
            token = Token::TK_GTE;
          } else if (c_ == '>') {
            Advance();
            if (c_ == '=') {
              Advance();
              token = Token::TK_ASSIGN_SAR;
            } else if (c_ == '>') {
              Advance();
              if (c_ == '=') {
                Advance();
                token = Token::TK_ASSIGN_SHR;
              } else {
                token = Token::TK_SHR;
              }
            } else {
              token = Token::TK_SAR;
            }
          } else {
            token = Token::TK_GT;
          }
          break;

        case '=':
          // = == ===
          Advance();
          if (c_ == '=') {
            Advance();
            if (c_ == '=') {
              Advance();
              token = Token::TK_EQ_STRICT;
            } else {
              token = Token::TK_EQ;
            }
          } else {
            token = Token::TK_ASSIGN;
          }
          break;

        case '!':
          // ! != !==
          Advance();
          if (c_ == '=') {
            Advance();
            if (c_ == '=') {
              Advance();
              token = Token::TK_NE_STRICT;
            } else {
              token = Token::TK_NE;
            }
          } else {
            token = Token::TK_NOT;
          }
          break;

        case '+':
          // + ++ +=
          Advance();
          if (c_ == '+') {
            Advance();
            token = Token::TK_INC;
          } else if (c_ == '=') {
            Advance();
            token = Token::TK_ASSIGN_ADD;
          } else {
            token = Token::TK_ADD;
          }
          break;

        case '-':
          // - -- --> -=
          Advance();
          if (c_ == '-') {
            Advance();
            if (c_ == '>' && has_line_terminator_before_next_) {
              token = SkipSingleLineComment<false>();
            } else {
              token = Token::TK_DEC;
            }
          } else if (c_ == '=') {
            Advance();
            token = Token::TK_ASSIGN_SUB;
          } else {
            token = Token::TK_SUB;
          }
          break;

        case '*':
          // * *=
          Advance();
          if (c_ == '=') {
            Advance();
            token = Token::TK_ASSIGN_MUL;
          } else {
            token = Token::TK_MUL;
          }
          break;

        case '%':
          // % %=
          Advance();
          if (c_ == '=') {
            Advance();
            token = Token::TK_ASSIGN_MOD;
          } else {
            token = Token::TK_MOD;
          }
          break;

        case '/':
          // / // /* /=
          // ASSIGN_DIV and DIV remain to be solved which is RegExp or not.
          Advance();
          if (c_ == '/') {
            // SINGLE LINE COMMENT
            token = SkipSingleLineComment<RecognizeCommentAsToken>();
          } else if (c_ == '*') {
            // MULTI LINES COMMENT
            token = SkipMultiLineComment<RecognizeCommentAsToken>();
          } else if (c_ == '=') {
            // ASSIGN_DIV
            Advance();
            token = Token::TK_ASSIGN_DIV;
          } else {
            // DIV
            token = Token::TK_DIV;
          }
          break;

        case '&':
          // && &= &
          Advance();
          if (c_ == '&') {
            Advance();
            token = Token::TK_LOGICAL_AND;
          } else if (c_ == '=') {
            Advance();
            token = Token::TK_ASSIGN_BIT_AND;
          } else {
            token = Token::TK_BIT_AND;
          }
          break;

        case '|':
          // || |= |
          Advance();
          if (c_ == '|') {
            Advance();
            token = Token::TK_LOGICAL_OR;
          } else if (c_ == '=') {
            Advance();
            token = Token::TK_ASSIGN_BIT_OR;
          } else {
            token = Token::TK_BIT_OR;
          }
          break;

        case '^':
          // ^
          Advance();
          if (c_ == '=') {
            Advance();
            token = Token::TK_ASSIGN_BIT_XOR;
          } else {
            token = Token::TK_BIT_XOR;
          }
          break;

        case '.':
          // . Number
          Advance();
          if (c_ >= 0 && character::IsDecimalDigit(c_)) {
            // float number parse
            token = ScanNumber<true>();
          } else {
            token = Token::TK_PERIOD;
          }
          break;

        case ':':
          Advance();
          token = Token::TK_COLON;
          break;

        case ';':
          Advance();
          token = Token::TK_SEMICOLON;
          break;

        case ',':
          Advance();
          token = Token::TK_COMMA;
          break;

        case '(':
          Advance();
          token = Token::TK_LPAREN;
          break;

        case ')':
          Advance();
          token = Token::TK_RPAREN;
          break;

        case '[':
          Advance();
          token = Token::TK_LBRACK;
          break;

        case ']':
          Advance();
          token = Token::TK_RBRACK;
          break;

        case '{':
          Advance();
          token = Token::TK_LBRACE;
          break;

        case '}':
          Advance();
          token = Token::TK_RBRACE;
          break;

        case '?':
          Advance();
          token = Token::TK_CONDITIONAL;
          break;

        case '~':
          Advance();
          token = Token::TK_BIT_NOT;
          break;

        default:
          if (c_ < 0) {
            // EOS
            token = Token::TK_EOS;
          } else if (character::IsIdentifierStart(c_)) {
            token = ScanIdentifier<LexType>(strict);
          } else if (character::IsDecimalDigit(c_)) {
            token = ScanNumber<false>();
          } else if (character::IsLineTerminator(c_)) {
            SkipLineTerminator();
            has_line_terminator_before_next_ = true;
            token = Token::TK_NOT_FOUND;
          } else {
            token = Token::TK_ILLEGAL;
          }
          break;
      }
    } while (token == Token::TK_NOT_FOUND);
    if (c_ == -1) {
      location_.set_end_position(pos());
    } else {
      location_.set_end_position(pos() - 1);
    }
    return token;
  }

  inline const std::vector<uint16_t>& Buffer() const {
    return buffer16_;
  }

  inline const std::vector<char>& Buffer8() const {
    return buffer8_;
  }

  inline const double& Numeric() const {
    return numeric_;
  }

  inline State NumericType() const {
    assert(type_ == DECIMAL || type_ == HEX || type_ == OCTAL);
    return type_;
  }

  inline State StringEscapeType() const {
    assert(type_ == NONE || type_ == ESCAPE || type_ == OCTAL);
    return type_;
  }

  inline bool has_line_terminator_before_next() const {
    return has_line_terminator_before_next_;
  }

  std::size_t line_number() const {
    return line_number_;
  }

  std::string filename() const {
    return SourceTraits<Source>::GetFileName(source_);
  }

  std::size_t pos() const {
    return pos_;
  }

  inline const Source& source() const {
    return source_;
  }

  inline const Location& location() const {
    return location_;
  }

  inline std::size_t begin_position() const {
    return location_.begin_position();
  }

  inline std::size_t end_position() const {
    return location_.end_position();
  }

  inline std::size_t previous_begin_position() const {
    return previous_location_.begin_position();
  }

  inline std::size_t previous_end_position() const {
    return previous_location_.end_position();
  }

  bool ScanRegExpLiteral(bool contains_eq) {
    // location begin_position is the same with DIV
    // so, no need to set
    bool character = false;
    buffer16_.clear();
    if (contains_eq) {
      Record16('=');
    }
    while (c_ != '/' || character) {
      // invalid RegExp pattern
      if (c_ < 0 || character::IsLineTerminator(c_)) {
        return false;
      }
      if (c_ == '\\') {
        // escape
        Record16Advance();
        if (c_ < 0 || character::IsLineTerminator(c_)) {
          return false;
        }
        Record16Advance();
      } else {
        if (c_ == '[') {
          character = true;
        } else if (c_ == ']') {
          character = false;
        }
        Record16Advance();
      }
    }
    Advance();  // waste '/'
    return true;
  }

  bool ScanRegExpFlags() {
    buffer16_.clear();
    while (c_ >= 0 && character::IsIdentifierPart(c_)) {
      if (c_ == '\\') {
        Advance();
        if (c_ != 'u') {
          return false;
        }
        Advance();
        bool ng = false;
        const uint16_t uc = ScanHexEscape('u', 4, &ng);
        if (ng || uc == '\\') {
          return false;
        }
        Record16(uc);
      } else {
        Record16Advance();
      }
    }
    if (c_ == -1) {
      location_.set_end_position(pos());
    } else {
      location_.set_end_position(pos() - 1);
    }
    return true;
  }

 private:
  static const std::size_t kInitialReadBufferCapacity = 32;

  inline void StorePreviousLocation() {
    previous_location_ = location_;
  }

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

  inline void Record16() {
    buffer16_.push_back(c_);
  }

  inline void Record16(const int ch) {
    buffer16_.push_back(ch);
  }

  inline void Record8Advance() {
    Record8();
    Advance();
  }

  inline void Record16Advance() {
    Record16();
    Advance();
  }

  void PushBack() {
    if (pos_ < 2) {
      c_ = -1;
    } else {
      c_ = source_[pos_-2];
      --pos_;
    }
  }

  template<bool Val>
  Token::Type SkipSingleLineComment(typename disable_if_c<Val>::type* = 0) {
    Advance();
    // see ECMA-262 section 7.4
    while (c_ >= 0 && !character::IsLineTerminator(c_)) {
      Advance();
    }
    return Token::TK_NOT_FOUND;
  }

  template<bool Val>
  Token::Type SkipSingleLineComment(typename enable_if_c<Val>::type* = 0) {
    Advance();
    // see ECMA-262 section 7.4
    while (c_ >= 0 && !character::IsLineTerminator(c_)) {
      Advance();
    }
    return Token::TK_SINGLE_LINE_COMMENT;
  }

  template<bool Val>
  Token::Type SkipMultiLineComment(typename disable_if_c<Val>::type* = 0) {
    Advance();
    // remember previous ch
    uint16_t ch;
    while (c_ >= 0) {
      ch = c_;
      Advance();
      if (ch == '*' && c_ == '/') {
        c_ = ' ';
        return Token::TK_NOT_FOUND;
      } else if (c_ >= 0 && character::IsLineTerminator(c_)) {
        // see ECMA-262 section 7.4
        SkipLineTerminator();
        has_line_terminator_before_next_ = true;
        ch = '\n';
      }
    }
    return Token::TK_ILLEGAL;
  }

  template<bool Val>
  Token::Type SkipMultiLineComment(typename enable_if_c<Val>::type* = 0) {
    Advance();
    // remember previous ch
    uint16_t ch;
    while (c_ >= 0) {
      ch = c_;
      Advance();
      if (ch == '*' && c_ == '/') {
        c_ = ' ';
        return Token::TK_MULTI_LINE_COMMENT;
      } else if (c_ >= 0 && character::IsLineTerminator(c_)) {
        // see ECMA-262 section 7.4
        SkipLineTerminator();
        has_line_terminator_before_next_ = true;
        ch = '\n';
      }
    }
    return Token::TK_ILLEGAL;
  }

  Token::Type SkipHtmlComment() {
    Advance();
    if (c_ == '-') {
      // <!-
      Advance();
      if (c_ == '-') {
        // <!--
        return SkipSingleLineComment<false>();
      }
      PushBack();
    }
    // <! is LT and NOT
    PushBack();
    return Token::TK_LT;
  }

  template<typename LexType>
  Token::Type ScanIdentifier(bool strict) {
    buffer16_.clear();

    if (c_ == '\\') {
      Advance();
      if (c_ != 'u') {
        return Token::TK_ILLEGAL;
      }
      Advance();
      bool ng = false;
      const uint16_t uc = ScanHexEscape('u', 4, &ng);
      if (ng || uc == '\\' || !character::IsIdentifierStart(uc)) {
        return Token::TK_ILLEGAL;
      }
      Record16(uc);
    } else {
      Record16Advance();
    }

    while (c_ >= 0 && character::IsIdentifierPart(c_)) {
      if (c_ == '\\') {
        Advance();
        if (c_ != 'u') {
          return Token::TK_ILLEGAL;
        }
        Advance();
        bool ng = false;
        const uint16_t uc = ScanHexEscape('u', 4, &ng);
        if (ng || uc == '\\' || !character::IsIdentifierPart(uc)) {
          return Token::TK_ILLEGAL;
        }
        Record16(uc);
      } else {
        Record16Advance();
      }
    }

    return detail::Keyword<LexType>::Detect(buffer16_, strict);
  }

  Token::Type ScanString() {
    type_ = NONE;
    const uint16_t quote = c_;
    buffer16_.clear();
    Advance();
    while (c_ != quote && c_ >= 0 && !character::IsLineTerminator(c_)) {
      if (c_ == '\\') {
        Advance();
        // escape sequence
        if (c_ < 0) return Token::TK_ILLEGAL;
        if (type_ == NONE) {
          type_ = ESCAPE;
        }
        if (!ScanEscape()) {
          return Token::TK_ILLEGAL;
        }
      } else {
        Record16Advance();
      }
    }
    if (c_ != quote) {
      // not closed
      return Token::TK_ILLEGAL;
    }
    Advance();

    return Token::TK_STRING;
  }

  bool ScanEscape() {
    if (c_ >= 0 && character::IsLineTerminator(c_)) {
      SkipLineTerminator();
      return true;
    }
    switch (c_) {
      case '\'':
      case '"' :
      case '\\':
        Record16Advance();
        break;

      case 'b' :
        Record16('\b');
        Advance();
        break;

      case 'f' :
        Record16('\f');
        Advance();
        break;

      case 'n' :
        Record16('\n');
        Advance();
        break;

      case 'r' :
        Record16('\r');
        Advance();
        break;

      case 't' :
        Record16('\t');
        Advance();
        break;

      case 'u' : {
        Advance();
        bool ng = false;
        const uint16_t uc = ScanHexEscape('u', 4, &ng);
        if (ng) {
          return false;
        }
        Record16(uc);
        break;
      }

      case 'v' :
        Record16('\v');
        Advance();
        break;

      case 'x' : {
        Advance();
        bool ng = false;
        const uint16_t uc = ScanHexEscape('x', 2, &ng);
        if (ng) {
          return false;
        }
        Record16(uc);
        break;
      }

      case '1' :
      case '2' :
      case '3' : {
        if (type_ != OCTAL) {
          type_ = OCTAL;
        }
        // fall through
      }

      case '0' : {
        uint16_t uc = OctalValue(c_);
        Advance();
        if (c_ >= 0 && character::IsDecimalDigit(c_)) {
          if (!character::IsOctalDigit(c_)) {
            // invalid
            return false;
          }
          if (type_ != OCTAL) {
            type_ = OCTAL;
          }
          uc = uc * 8 + OctalValue(c_);
          Advance();
          if (c_ >= 0 && character::IsDecimalDigit(c_)) {
            if (!character::IsOctalDigit(c_)) {
              // invalid
              return false;
            }
            uc = uc * 8 + OctalValue(c_);
            Advance();
          }
        }
        Record16(uc);
        break;
      }

      case '4' :
      case '5' :
      case '6' :
      case '7' : {
        if (type_ != OCTAL) {
          type_ = OCTAL;
        }
        uint16_t uc = OctalValue(c_);
        Advance();
        if (c_ >= 0 && character::IsDecimalDigit(c_)) {
          if (!character::IsOctalDigit(c_)) {
            // invalid
            return false;
          }
          uc = uc * 8 + OctalValue(c_);
          Advance();
        }
        Record16(uc);
        break;
      }

      case '8' :
      case '9' :
        // section 7.8.4 and B1.2
        return false;

      default:
        Record16Advance();
        break;
    }
    return true;
  }

  template<bool period>
  Token::Type ScanNumber() {
    buffer8_.clear();
    State type = DECIMAL;
    bool is_decimal_integer = !period;
    if (period) {
      Record8('0');
      Record8('.');
      ScanDecimalDigits();
    } else {
      if (c_ == '0') {
        // 0x (hex) or 0 (octal)
        Record8Advance();
        if (c_ == 'x' || c_ == 'X') {
          // 0x (hex)
          type = HEX;
          Record8Advance();
          if (c_ < 0 || !character::IsHexDigit(c_)) {
            return Token::TK_ILLEGAL;
          }
          while (c_ >= 0 && character::IsHexDigit(c_)) {
            Record8Advance();
          }
        } else if (c_ >= 0 && character::IsOctalDigit(c_)) {
          // 0 (octal)
          // octal number cannot convert with strtod
          type = OCTAL;
          Record8Advance();
          while (true) {
            if (c_ < '0' || '7' < c_) {
              break;
            }
            Record8Advance();
          }
        }
      } else {
        ScanDecimalDigits();
      }
      if (type == DECIMAL && c_ == '.') {
        is_decimal_integer = false;
        Record8Advance();
        ScanDecimalDigits();
      }
    }

    // exponent part
    if (c_ == 'e' || c_ == 'E') {
      is_decimal_integer = false;
      if (type != DECIMAL) {
        return Token::TK_ILLEGAL;
      }
      Record8Advance();
      if (c_ == '+' || c_ == '-') {
        Record8Advance();
      }
      // more than 1 decimal digit required
      if (c_ < 0 || !character::IsDecimalDigit(c_)) {
        return Token::TK_ILLEGAL;
      }
      ScanDecimalDigits();
    }

    // see ECMA-262 section 7.8.3
    // "immediately following a NumericLiteral must not be an IdentifierStart or
    // DecimalDigit."
    if (c_ >= 0 && (character::IsDecimalDigit(c_) || character::IsIdentifierStart(c_))) {
      return Token::TK_ILLEGAL;
    }


    if (type == DECIMAL) {
      if (is_decimal_integer) {
        numeric_ = ParseIntegerOverflow(buffer8_.begin(),
                                        buffer8_.end(),
                                        10);
      } else {
        const std::string buf(buffer8_.begin(), buffer8_.end());
        numeric_ = std::atof(buf.c_str());
      }
    } else if (type == HEX) {
      assert(buffer8_.size() > 2);  // first 0x
      numeric_ = ParseIntegerOverflow(buffer8_.begin() + 2,
                                      buffer8_.end(),
                                      16);
    } else {
      assert(type == OCTAL);
      assert(buffer8_.size() > 1);  // first 0
      numeric_ = ParseIntegerOverflow(buffer8_.begin() + 1,
                                      buffer8_.end(),
                                      8);
    }
    type_ = type;
    return Token::TK_NUMBER;
  }

  uint16_t ScanOctalEscape() {
    uint16_t res = 0;
    for (int i = 0; i < 3; ++i) {
      const int d = OctalValue(c_);
      if (d < 0) {
        break;
      }
      const int t = res * 8 + d;
      if (t > 255) {
        break;
      }
      res = t;
      Advance();
    }
    return res;
  }

  uint16_t ScanHexEscape(uint16_t c, int len, bool* ng) {
    uint16_t res = 0;
    for (int i = 0; i < len; ++i) {
      const int d = HexValue(c_);
      if (d < 0) {
        for (int j = i - 1; j >= 0; --j) {
          PushBack();
        }
        *ng = true;
        return c;
      }
      res = res * 16 + d;
      Advance();
    }
    return res;
  }

  void ScanDecimalDigits() {
    while (c_ >= 0 && character::IsDecimalDigit(c_)) {
      Record8Advance();
    }
  }

  void SkipLineTerminator() {
    const uint16_t c = c_;
    Advance();
    if (c + c_ == '\n' + '\r') {
      Advance();
    }
    ++line_number_;
  }

  const Source& source_;
  std::vector<char> buffer8_;
  std::vector<uint16_t> buffer16_;
  double numeric_;
  State type_;
  std::size_t pos_;
  const std::size_t end_;
  bool has_line_terminator_before_next_;
  bool has_shebang_;
  int c_;
  std::size_t line_number_;
  Location previous_location_;
  Location location_;
};


} }  // namespace iv::core
#endif  // _IV_LEXER_H_
