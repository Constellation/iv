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

namespace iv {
namespace core {

template<typename Source>
class Lexer: private Noncopyable<Lexer<Source> >::type {
 public:
  enum LexType {
    kClear = 0,
    kIdentifyReservedWords = 1,
    kIgnoreReservedWords = 2,
    kIgnoreReservedWordsAndIdentifyGetterOrSetter = 4,
    kStrict = 8
  };
  enum State {
    NONE,
    ESCAPE,
    DECIMAL,
    HEX,
    OCTAL
  };

  explicit Lexer(Source* src)
      : source_(src),
        buffer8_(),
        buffer16_(kInitialReadBufferCapacity),
        pos_(0),
        end_(source_->size()),
        has_line_terminator_before_next_(false),
        has_shebang_(false),
        line_number_(1),
        location_() {
    Initialize();
  }

  typename Token::Type Next(int type) {
    typename Token::Type token;
    has_line_terminator_before_next_ = false;
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
            token = ScanNumber(true);
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
            token = ScanIdentifier(type);
          } else if (Chars::IsDecimalDigit(c_)) {
            token = ScanNumber(false);
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
    location_.set_end_position(pos() - 1);
    return token;
  }

  inline const std::vector<uc16>& Buffer() const {
    return buffer16_;
  }

  inline const std::string& Buffer8() const {
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

  inline Source* source() const {
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
    location_.set_end_position(pos() - 1);
    return true;
  }

 private:
  static const std::size_t kInitialReadBufferCapacity = 32;

  void Initialize() {
    Advance();
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

  void PushBack() {
    if (pos_ < 2) {
      c_ = -1;
    } else {
      c_ = source_->Get(pos_-2);
      --pos_;
    }
  }

  inline Token::Type IsMatch(char const * keyword,
                             std::size_t len,
                             Token::Type guess, bool strict) const {
    if (!strict) {
      return Token::IDENTIFIER;
    }
    std::vector<uc16>::const_iterator it = buffer16_.begin();
    do {
      if (*it++ != *keyword++) {
        return Token::IDENTIFIER;
      }
    } while (--len);
    return guess;
  }

  inline Token::Type IsMatch(char const * keyword,
                             std::size_t len,
                             Token::Type guess) const {
    std::vector<uc16>::const_iterator it = buffer16_.begin();
    do {
      if (*it++ != *keyword++) {
        return Token::IDENTIFIER;
      }
    } while (--len);
    return guess;
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

  Token::Type ScanIdentifier(int type) {
    Token::Type token = Token::IDENTIFIER;
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

    if (type & kIdentifyReservedWords) {
      token = DetectKeyword(type & kStrict);
    } else if (type & kIgnoreReservedWordsAndIdentifyGetterOrSetter) {
      token = DetectGetOrSet();
    }

    return token;
  }

  // detect which Identifier is Keyword, FutureReservedWord or not
  // Keyword and FutureReservedWord are defined in ECMA-262 5th.
  //
  // Some words such as :
  // int, short, boolean, byte, long, char, float, double, abstract, volatile,
  // transient, final, throws, goto, native, synchronized
  // were defined as FutureReservedWord in ECMA-262 3rd, but not in 5th.
  // So, DetectKeyword interprets them as Identifier.
  Token::Type DetectKeyword(bool strict) const {
    const std::size_t len = buffer16_.size();
    Token::Type token = Token::IDENTIFIER;
    switch (len) {
      case 2:
        // if in do
        if (buffer16_[0] == 'i') {
          if (buffer16_[1] == 'f') {
            token = Token::IF;
          } else if (buffer16_[1] == 'n') {
            token = Token::IN;
          }
        } else if (buffer16_[0] == 'd' && buffer16_[1] == 'o') {
          // do
          token = Token::DO;
        }
        break;
      case 3:
        // for var int new try let
        switch (buffer16_[2]) {
          case 't':
            if (buffer16_[0] == 'l' && buffer16_[1] == 'e' && strict) {
              // let
              token = Token::LET;
            } else if (buffer16_[0] == 'i' && buffer16_[1] == 'n') {
              // int (removed)
              // token = Token::INT;
            }
            break;
          case 'r':
            // for var
            if (buffer16_[0] == 'f' && buffer16_[1] == 'o') {
              // for
              token = Token::FOR;
            } else if (buffer16_[0] == 'v' && buffer16_[1] == 'a') {
              // var
              token = Token::VAR;
            }
            break;
          case 'y':
            // try
            if (buffer16_[0] == 't' && buffer16_[1] == 'r') {
              token = Token::TRY;
            }
            break;
          case 'w':
            // new
            if (buffer16_[0] == 'n' && buffer16_[1] == 'e') {
              token = Token::NEW;
            }
            break;
        }
        break;
      case 4:
        // else case true byte null this
        // void with long enum char goto
        // number 3 character is most duplicated
        switch (buffer16_[3]) {
          case 'e':
            // else case true byte
            if (buffer16_[2] == 's') {
              if (buffer16_[0] == 'e' && buffer16_[1] == 'l') {
                // else
                token = Token::ELSE;
              } else if (buffer16_[0] == 'c' && buffer16_[1] == 'a') {
                // case
                token = Token::CASE;
              }
            } else if (buffer16_[0] == 't' &&
                       buffer16_[1] == 'r' && buffer16_[2] == 'u') {
              // true
              token = Token::TRUE_LITERAL;
            } else if (buffer16_[0] == 'b' &&
                       buffer16_[1] == 'y' && buffer16_[2] == 't') {
              // byte (removed)
              // token = Token::BYTE;
            }
            break;
          case 'l':
            // null
            if (buffer16_[0] == 'n' &&
                buffer16_[1] == 'u' && buffer16_[2] == 'l') {
              token = Token::NULL_LITERAL;
            }
            break;
          case 's':
            // this
            if (buffer16_[0] == 't' &&
                buffer16_[1] == 'h' && buffer16_[2] == 'i') {
              token = Token::THIS;
            }
            break;
          case 'd':
            // void
            if (buffer16_[0] == 'v' &&
                buffer16_[1] == 'o' && buffer16_[2] == 'i') {
              token = Token::VOID;
            }
            break;
          case 'h':
            // with
            if (buffer16_[0] == 'w' &&
                buffer16_[1] == 'i' && buffer16_[2] == 't') {
              token = Token::WITH;
            }
            break;
          case 'g':
            // long (removed)
            if (buffer16_[0] == 'l' &&
                buffer16_[1] == 'o' && buffer16_[2] == 'n') {
              // token = Token::LONG;
            }
            break;
          case 'm':
            // enum
            if (buffer16_[0] == 'e' &&
                buffer16_[1] == 'n' && buffer16_[2] == 'u') {
              token = Token::ENUM;
            }
            break;
          case 'r':
            // char (removed)
            if (buffer16_[0] == 'c' &&
                buffer16_[1] == 'h' && buffer16_[2] == 'a') {
              // token = Token::CHAR;
            }
            break;
          case 'o':
            // goto (removed)
            if (buffer16_[0] == 'g' &&
                buffer16_[1] == 'o' && buffer16_[2] == 't') {
              // token = Token::GOTO;
            }
            break;
        }
        break;
      case 5:
        // break final float catch super while
        // throw short class const false yield
        // number 3 character is most duplicated
        switch (buffer16_[3]) {
          case 'a':
            // break final float
            if (buffer16_[0] == 'b' && buffer16_[1] == 'r' &&
                buffer16_[2] == 'e' && buffer16_[4] == 'k') {
              // break
              token = Token::BREAK;
            } else if (buffer16_[0] == 'f') {
              if (buffer16_[1] == 'i' &&
                  buffer16_[2] == 'n' && buffer16_[4] == 'l') {
                // final (removed)
                // token = Token::FINAL;
              } else if (buffer16_[1] == 'l' &&
                         buffer16_[2] == 'o' && buffer16_[4] == 't') {
                // float (removed)
                // token = Token::FLOAT;
              }
            }
            break;
          case 'c':
            if (buffer16_[0] == 'c' && buffer16_[1] == 'a' &&
                buffer16_[2] == 't' && buffer16_[4] == 'h') {
              // catch
              token = Token::CATCH;
            }
            break;
          case 'e':
            if (buffer16_[0] == 's' && buffer16_[1] == 'u' &&
                buffer16_[2] == 'p' && buffer16_[4] == 'r') {
              // super
              token = Token::SUPER;
            }
            break;
          case 'l':
            if (buffer16_[0] == 'w' && buffer16_[1] == 'h' &&
                buffer16_[2] == 'i' && buffer16_[4] == 'e') {
              // while
              token = Token::WHILE;
            } else if (strict &&
                       buffer16_[0] == 'y' && buffer16_[1] == 'i' &&
                       buffer16_[2] == 'e' && buffer16_[4] == 'd') {
              // yield
              token = Token::YIELD;
            }
            break;
          case 'o':
            if (buffer16_[0] == 't' && buffer16_[1] == 'h' &&
                buffer16_[2] == 'r' && buffer16_[4] == 'w') {
              // throw
              token = Token::THROW;
            }
            break;
          case 'r':
            if (buffer16_[0] == 's' && buffer16_[1] == 'h' &&
                buffer16_[2] == 'o' && buffer16_[4] == 't') {
              // short (removed)
              // token = Token::SHORT;
            }
            break;
          case 's':
            // class const false
            if (buffer16_[0] == 'c') {
              if (buffer16_[1] == 'l' &&
                  buffer16_[2] == 'a' && buffer16_[4] == 's') {
                // class
                token = Token::CLASS;
              } else if (buffer16_[1] == 'o' &&
                         buffer16_[2] == 'n' && buffer16_[4] == 't') {
                // const
                token = Token::CONST;
              }
            } else if (buffer16_[0] == 'f' && buffer16_[1] == 'a' &&
                       buffer16_[2] == 'l' && buffer16_[4] == 'e') {
              // false
              token = Token::FALSE_LITERAL;
            }
            break;
        }
        break;
      case 6:
        // double delete export import native
        // public return static switch typeof throws
        // number 0 character is most duplicated
        switch (buffer16_[0]) {
          case 'd':
            // double delete
            if (buffer16_[5] == 'e' &&
                buffer16_[4] == 'l' && buffer16_[3] == 'b' &&
                buffer16_[2] == 'u' && buffer16_[1] == 'o') {
              // double
              // token = Token::DOUBLE;
            } else if (buffer16_[5] == 'e' &&
                       buffer16_[4] == 't' && buffer16_[3] == 'e' &&
                       buffer16_[2] == 'l' && buffer16_[1] == 'e') {
              // delete
              token = Token::DELETE;
            }
            break;
          case 'e':
            // export
            token = IsMatch("export", len, Token::EXPORT);
            break;
          case 'i':
            // import
            token = IsMatch("import", len, Token::IMPORT);
            break;
          case 'n':
            // native (removed)
            // token = IsMatch("native", len, Token::NATIVE);
            break;
          case 'p':
            // public
            token = IsMatch("public", len, Token::PUBLIC, strict);
            break;
          case 'r':
            // return
            token = IsMatch("return", len, Token::RETURN);
            break;
          case 's':
            // switch static
            if (buffer16_[1] == 'w' &&
                buffer16_[2] == 'i' && buffer16_[3] == 't' &&
                buffer16_[4] == 'c' && buffer16_[5] == 'h') {
              // switch
              token = Token::SWITCH;
            } else if (strict &&
                       buffer16_[1] == 't' &&
                       buffer16_[2] == 'a' && buffer16_[3] == 't' &&
                       buffer16_[4] == 'i' && buffer16_[5] == 'c') {
              // static
              token = Token::STATIC;
            }
            break;
          case 't':
            // typeof throws
            if (buffer16_[5] == 'f' &&
                buffer16_[4] == 'o' && buffer16_[3] == 'e' &&
                buffer16_[2] == 'p' && buffer16_[1] == 'y') {
              // typeof
              token = Token::TYPEOF;
            } else if (buffer16_[5] == 's' &&
                       buffer16_[4] == 'w' && buffer16_[3] == 'o' &&
                       buffer16_[2] == 'r' && buffer16_[1] == 'h') {
              // throws (removed)
              // token = Token::THROWS;
            }
            break;
        }
        break;
      case 7:
        // boolean default extends finally package private
        // number 0 character is most duplicated
        switch (buffer16_[0]) {
          case 'b':
            // boolean (removed)
            // token = IsMatch("boolean", len, Token::BOOLEAN);
            break;
          case 'd':
            token = IsMatch("default", len, Token::DEFAULT);
            break;
          case 'e':
            token = IsMatch("extends", len, Token::EXTENDS);
            break;
          case 'f':
            token = IsMatch("finally", len, Token::FINALLY);
            break;
          case 'p':
            if (buffer16_[1] == 'a') {
              token = IsMatch("package", len, Token::PACKAGE, strict);
            } else if (buffer16_[1] == 'r') {
              token = IsMatch("private", len, Token::PRIVATE, strict);
            }
            break;
        }
        break;
      case 8:
        // debugger continue abstract volatile function
        // number 4 character is most duplicated
        switch (buffer16_[4]) {
          case 'g':
            token = IsMatch("debugger", len, Token::DEBUGGER);
            break;
          case 'i':
            token = IsMatch("continue", len, Token::CONTINUE);
            break;
          case 'r':
            // abstract (removed)
            // token = IsMatch("abstract", len, Token::ABSTRACT);
            break;
          case 't':
            if (buffer16_[1] == 'o') {
              // token = IsMatch("volatile", len, Token::VOLATILE);
            } else if (buffer16_[1] == 'u') {
              token = IsMatch("function", len, Token::FUNCTION);
            }
            break;
        }
        break;
      case 9:
        // interface protected transient
        if (buffer16_[1] == 'n') {
          token = IsMatch("interface", len, Token::INTERFACE, strict);
        } else if (buffer16_[1] == 'r') {
          if (buffer16_[0] == 'p') {
            token = IsMatch("protected", len, Token::PROTECTED, strict);
          } else if (buffer16_[0] == 't') {
            // transient (removed)
            // token = IsMatch("transient", len, Token::TRANSIENT);
          }
        }
        break;
      case 10:
        // instanceof implements
        if (buffer16_[1] == 'n') {
          token = IsMatch("instanceof", len, Token::INSTANCEOF);
        } else if (buffer16_[1] == 'm') {
          token = IsMatch("implements", len, Token::IMPLEMENTS, strict);
        }
        break;
      case 12:
        // synchronized (removed)
        // token = IsMatch("synchronized", len, Token::SYNCHRONIZED);
        token = Token::IDENTIFIER;
        break;
    }
    return token;
  }

  Token::Type DetectGetOrSet() const {
    if (buffer16_.size() == 3) {
      if (buffer16_[1] == 'e' && buffer16_[2] == 't') {
        if (buffer16_[0] == 'g') {
          return Token::GET;
        } else if (buffer16_[0] == 's') {
          return Token::SET;
        }
      }
    }
    return Token::IDENTIFIER;
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
      case '0' :
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

  Token::Type ScanNumber(const bool period) {
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
            if (c_ == '8' || c_ == '9') {
              // not octal digits
              type = DECIMAL;
              break;
            }
            if (c_ < '0' || '7' < c_) {
              break;
            }
            Record8Advance();
          }
        }
      }
      if (type == DECIMAL) {
        ScanDecimalDigits();
        if (c_ == '.') {
          Record8Advance();
          ScanDecimalDigits();
        }
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

    if (type == OCTAL) {
      double val = 0;
      for (std::string::const_iterator it = buffer8_.begin(),
           last = buffer8_.end(); it != last; ++it) {
        val = val * 8 + (*it - '0');
      }
      numeric_ = val;
    } else {
      numeric_ = std::strtod(buffer8_.c_str(), NULL);
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

  inline int OctalValue(const int c) const {
    if ('0' <= c && c <= '8') {
      return c - '0';
    }
    return -1;
  }

  inline int HexValue(const int c) const {
    if ('0' <= c && c <= '9') {
      return c - '0';
    }
    if ('a' <= c && c <= 'f') {
      return c - 'a' + 10;
    }
    if ('A' <= c && c <= 'F') {
      return c - 'A' + 10;
    }
    return -1;
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

  Source* source_;
  std::string buffer8_;
  std::vector<uc16> buffer16_;
  double numeric_;
  State type_;
  std::size_t pos_;
  const std::size_t end_;
  bool has_line_terminator_before_next_;
  bool has_shebang_;
  int c_;
  std::size_t line_number_;
  Location location_;
};


} }  // namespace iv::core
#endif  // _IV_LEXER_H_
