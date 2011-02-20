#ifndef _IV_LEXER_H_
#define _IV_LEXER_H_

#include <cstddef>
#include <cassert>
#include <cstdlib>
#include <vector>
#include <string>
#include "uchar.h"
#include "chars.h"
#include "token.h"
#include "location.h"
#include "noncopyable.h"
#include "keyword.h"
#include "conversions.h"

namespace iv {
namespace core {

template<typename Source>
class Lexer: private Noncopyable<Lexer<Source> >::type {
 public:

  enum State {
    NONE,
    ESCAPE,
    DECIMAL,
    HEX,
    OCTAL
  };

  explicit Lexer(const Source* src)
      : source_(src),
        buffer8_(),
        buffer16_(kInitialReadBufferCapacity),
        pos_(0),
        end_(source_->size()),
        has_line_terminator_before_next_(false),
        has_shebang_(false),
        line_number_(1),
        previous_location_(),
        location_() {
    Initialize();
  }

  template<typename LexType>
  typename Token::Type Next(bool strict) {
    typename Token::Type token;
    has_line_terminator_before_next_ = false;
    StorePreviousLocation();
    do {
      while (Chars::IsWhiteSpace(c_)) {
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
            token = Token::LTE;
          } else if (c_ == '<') {
            Advance();
            if (c_ == '=') {
              Advance();
              token = Token::ASSIGN_SHL;
            } else {
              token = Token::SHL;
            }
          } else if (c_ == '!') {
            token = ScanHtmlComment();
          } else {
            token = Token::LT;
          }
          break;

        case '>':
          // > >= >> >>= >>> >>>=
          Advance();
          if (c_ == '=') {
            Advance();
            token = Token::GTE;
          } else if (c_ == '>') {
            Advance();
            if (c_ == '=') {
              Advance();
              token = Token::ASSIGN_SAR;
            } else if (c_ == '>') {
              Advance();
              if (c_ == '=') {
                Advance();
                token = Token::ASSIGN_SHR;
              } else {
                token = Token::SHR;
              }
            } else {
              token = Token::SAR;
            }
          } else {
            token = Token::GT;
          }
          break;

        case '=':
          // = == ===
          Advance();
          if (c_ == '=') {
            Advance();
            if (c_ == '=') {
              Advance();
              token = Token::EQ_STRICT;
            } else {
              token = Token::EQ;
            }
          } else {
            token = Token::ASSIGN;
          }
          break;

        case '!':
          // ! != !==
          Advance();
          if (c_ == '=') {
            Advance();
            if (c_ == '=') {
              Advance();
              token = Token::NE_STRICT;
            } else {
              token = Token::NE;
            }
          } else {
            token = Token::NOT;
          }
          break;

        case '+':
          // + ++ +=
          Advance();
          if (c_ == '+') {
            Advance();
            token = Token::INC;
          } else if (c_ == '=') {
            Advance();
            token = Token::ASSIGN_ADD;
          } else {
            token = Token::ADD;
          }
          break;

        case '-':
          // - -- --> -=
          Advance();
          if (c_ == '-') {
            Advance();
            if (c_ == '>' && has_line_terminator_before_next_) {
              token = SkipSingleLineComment();
            } else {
              token = Token::DEC;
            }
          } else if (c_ == '=') {
            Advance();
            token = Token::ASSIGN_SUB;
          } else {
            token = Token::SUB;
          }
          break;

        case '*':
          // * *=
          Advance();
          if (c_ == '=') {
            Advance();
            token = Token::ASSIGN_MUL;
          } else {
            token = Token::MUL;
          }
          break;

        case '%':
          // % %=
          Advance();
          if (c_ == '=') {
            Advance();
            token = Token::ASSIGN_MOD;
          } else {
            token = Token::MOD;
          }
          break;

        case '/':
          // / // /* /=
          // ASSIGN_DIV and DIV remain to be solved which is RegExp or not.
          Advance();
          if (c_ == '/') {
            // SINGLE LINE COMMENT
            if (line_number_ == (has_shebang_ ? 1 : 2)) {
              // magic comment
              token = ScanMagicComment();
            } else {
              token = SkipSingleLineComment();
            }
          } else if (c_ == '*') {
            // MULTI LINES COMMENT
            token = SkipMultiLineComment();
          } else if (c_ == '=') {
            // ASSIGN_DIV
            Advance();
            token = Token::ASSIGN_DIV;
          } else {
            // DIV
            token = Token::DIV;
          }
          break;

        case '&':
          // && &= &
          Advance();
          if (c_ == '&') {
            Advance();
            token = Token::LOGICAL_AND;
          } else if (c_ == '=') {
            Advance();
            token = Token::ASSIGN_BIT_AND;
          } else {
            token = Token::BIT_AND;
          }
          break;

        case '|':
          // || |= |
          Advance();
          if (c_ == '|') {
            Advance();
            token = Token::LOGICAL_OR;
          } else if (c_ == '=') {
            Advance();
            token = Token::ASSIGN_BIT_OR;
          } else {
            token = Token::BIT_OR;
          }
          break;

        case '^':
          // ^
          Advance();
          if (c_ == '=') {
            Advance();
            token = Token::ASSIGN_BIT_XOR;
          } else {
            token = Token::BIT_XOR;
          }
          break;

        case '.':
          // . Number
          Advance();
          if (Chars::IsDecimalDigit(c_)) {
            // float number parse
            token = ScanNumber<true>();
          } else {
            token = Token::PERIOD;
          }
          break;

        case ':':
          Advance();
          token = Token::COLON;
          break;

        case ';':
          Advance();
          token = Token::SEMICOLON;
          break;

        case ',':
          Advance();
          token = Token::COMMA;
          break;

        case '(':
          Advance();
          token = Token::LPAREN;
          break;

        case ')':
          Advance();
          token = Token::RPAREN;
          break;

        case '[':
          Advance();
          token = Token::LBRACK;
          break;

        case ']':
          Advance();
          token = Token::RBRACK;
          break;

        case '{':
          Advance();
          token = Token::LBRACE;
          break;

        case '}':
          Advance();
          token = Token::RBRACE;
          break;

        case '?':
          Advance();
          token = Token::CONDITIONAL;
          break;

        case '~':
          Advance();
          token = Token::BIT_NOT;
          break;

        case '#':
          // #!
          // skip shebang as single line comment
          if (pos_ == 1) {
            assert(line_number_ == 1);
            Advance();
            if (c_ == '!') {
              // shebang
              has_shebang_ = true;
              token = SkipSingleLineComment();
              break;
            }
            PushBack();
          }

        default:
          if (Chars::IsIdentifierStart(c_)) {
            token = ScanIdentifier<LexType>(strict);
          } else if (Chars::IsDecimalDigit(c_)) {
            token = ScanNumber<false>();
          } else if (Chars::IsLineTerminator(c_)) {
            SkipLineTerminator();
            has_line_terminator_before_next_ = true;
            token = Token::NOT_FOUND;
          } else if (c_ < 0) {
            // EOS
            token = Token::EOS;
          } else {
            token = Token::ILLEGAL;
          }
          break;
      }
    } while (token == Token::NOT_FOUND);
    if (c_ == -1) {
      location_.set_end_position(pos());
    } else {
      location_.set_end_position(pos() - 1);
    }
    return token;
  }

  inline const std::vector<uc16>& Buffer() const {
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

  const std::string& filename() const {
    return source_->filename();
  }

  std::size_t pos() const {
    return pos_;
  }

  inline const Source* source() const {
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
      if (Chars::IsLineTerminator(c_) || c_ < 0) {
        return false;
      }
      if (c_ == '\\') {
        // escape
        Record16Advance();
        if (Chars::IsLineTerminator(c_) || c_ < 0) {
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
    uc16 uc;
    while (Chars::IsIdentifierPart(c_)) {
      if (c_ == '\\') {
        Advance();
        if (c_ != 'u') {
          return false;
        }
        Advance();
        bool ng = false;
        uc = ScanHexEscape('u', 4, &ng);
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

  void Initialize() {
    Advance();
  }

  void Initialize(const Source* src) {
    using std::swap;
    source_ = src;
    pos_ = 0;
    end_ = source_->size();
    has_line_terminator_before_next_ = false;
    has_shebang_ = false;
    line_number_ = 1;
    swap(location_, Location());
  }

  inline void StorePreviousLocation() {
    previous_location_ = location_;
  }

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
      c_ = source_->Get(pos_-2);
      --pos_;
    }
  }

  Token::Type SkipSingleLineComment() {
    Advance();
    // see ECMA-262 section 7.4
    while (c_ >= 0 && !Chars::IsLineTerminator(c_)) {
      Advance();
    }
    return Token::NOT_FOUND;
  }

  Token::Type SkipMultiLineComment() {
    Advance();
    // remember previous ch
    uc16 ch;
    while (c_ >= 0) {
      ch = c_;
      Advance();
      if (ch == '*' && c_ == '/') {
        c_ = ' ';
        return Token::NOT_FOUND;
      } else if (Chars::IsLineTerminator(c_)) {
        // see ECMA-262 section 7.4
        SkipLineTerminator();
        has_line_terminator_before_next_ = true;
        ch = '\n';
      }
    }
    return Token::ILLEGAL;
  }

  Token::Type ScanHtmlComment() {
    Advance();
    if (c_ == '-') {
      // <!-
      Advance();
      if (c_ == '-') {
        // <!--
        return SkipSingleLineComment();
      }
      PushBack();
    }
    // <! is LT and NOT
    PushBack();
    return Token::LT;
  }

  Token::Type ScanMagicComment() {
    Advance();
    // see ECMA-262 section 7.4
    while (c_ >= 0 && !Chars::IsLineTerminator(c_)) {
      Advance();
    }
    return Token::NOT_FOUND;
  }

  template<typename LexType>
  Token::Type ScanIdentifier(bool strict) {
    uc16 uc;

    buffer16_.clear();

    if (c_ == '\\') {
      Advance();
      if (c_ != 'u') {
        return Token::ILLEGAL;
      }
      Advance();
      bool ng = false;
      uc = ScanHexEscape('u', 4, &ng);
      if (ng || uc == '\\' || !Chars::IsIdentifierStart(uc)) {
        return Token::ILLEGAL;
      }
      Record16(uc);
    } else {
      Record16Advance();
    }

    while (Chars::IsIdentifierPart(c_)) {
      if (c_ == '\\') {
        Advance();
        if (c_ != 'u') {
          return Token::ILLEGAL;
        }
        Advance();
        bool ng = false;
        uc = ScanHexEscape('u', 4, &ng);
        if (ng || uc == '\\' || !Chars::IsIdentifierPart(uc)) {
          return Token::ILLEGAL;
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
    const uc16 quote = c_;
    buffer16_.clear();
    Advance();
    while (c_ != quote && c_ >= 0 && !Chars::IsLineTerminator(c_)) {
      if (c_ == '\\') {
        Advance();
        // escape sequence
        if (c_ < 0) return Token::ILLEGAL;
        if (type_ == NONE) {
          type_ = ESCAPE;
        }
        if (!ScanEscape()) {
          return Token::ILLEGAL;
        }
      } else {
        Record16Advance();
      }
    }
    if (c_ != quote) {
      // not closed
      return Token::ILLEGAL;
    }
    Advance();

    return Token::STRING;
  }

  bool ScanEscape() {
    if (Chars::IsLineTerminator(c_)) {
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
        const uc16 uc = ScanHexEscape('u', 4, &ng);
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
        const uc16 uc = ScanHexEscape('x', 2, &ng);
        if (ng) {
          return false;
        }
        Record16(uc);
        break;
      }

      case '0' : {
        if (type_ != OCTAL) {
          type_ = OCTAL;
        }
        Record16(ScanOctalEscape());
        break;
      }

      case '1' :
      case '2' :
      case '3' :
      case '4' :
      case '5' :
      case '6' :
      case '7' :
        if (type_ != OCTAL) {
          type_ = OCTAL;
        }
        Record16(ScanOctalEscape());
        break;

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
          if (!Chars::IsHexDigit(c_)) {
            return Token::ILLEGAL;
          }
          while (Chars::IsHexDigit(c_)) {
            Record8Advance();
          }
        } else if (Chars::IsOctalDigit(c_)) {
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
        Record8Advance();
        ScanDecimalDigits();
      }
    }

    // exponent part
    if (c_ == 'e' || c_ == 'E') {
      if (type != DECIMAL) {
        return Token::ILLEGAL;
      }
      Record8Advance();
      if (c_ == '+' || c_ == '-') {
        Record8Advance();
      }
      // more than 1 decimal digit required
      if (!Chars::IsDecimalDigit(c_)) {
        return Token::ILLEGAL;
      }
      ScanDecimalDigits();
    }

    // see ECMA-262 section 7.8.3
    // "immediately following a NumericLiteral must not be an IdentifierStart or
    // DecimalDigit."
    if (Chars::IsDecimalDigit(c_) || Chars::IsIdentifierStart(c_)) {
      return Token::ILLEGAL;
    }


    if (type == DECIMAL) {
      const std::string buf(buffer8_.begin(), buffer8_.end());
      numeric_ = std::atof(buf.c_str());
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
    return Token::NUMBER;
  }

  uc16 ScanOctalEscape() {
    uc16 res = 0;
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

  uc16 ScanHexEscape(uc16 c, int len, bool* ng) {
    uc16 res = 0;
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
    while (Chars::IsDecimalDigit(c_)) {
      Record8Advance();
    }
  }

  void SkipLineTerminator() {
    const uc16 c = c_;
    Advance();
    if (c + c_ == '\n' + '\r') {
      Advance();
    }
    ++line_number_;
  }

  const Source* source_;
  std::vector<char> buffer8_;
  std::vector<uc16> buffer16_;
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
