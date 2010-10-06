#include "lexer.h"
#include <cstdio>
#include <cassert>

namespace iv {
namespace core {

Lexer::Lexer(Source* src)
    : source_(src),
      buffer8_(kInitialReadBufferCapacity),
      buffer16_(kInitialReadBufferCapacity),
      pos_(0),
      end_(source_->size()),
      has_line_terminator_before_next_(false),
      has_shebang_(false),
      line_number_(1) {
  Initialize();
}

void Lexer::Initialize() {
  Advance();
}

Token::Type Lexer::Next(Lexer::LexType type) {
  Token::Type token;
  has_line_terminator_before_next_ = false;
  do {
    while (ICU::IsWhiteSpace(c_)) {
      // white space
      Advance();
    }
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
        token = Token::BIT_XOR;
        break;

      case '.':
        // . Number
        Advance();
        if (ICU::IsDecimalDigit(c_)) {
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
        if (ICU::IsIdentifierStart(c_)) {
          token = ScanIdentifier(type);
        } else if (ICU::IsDecimalDigit(c_)) {
          token = ScanNumber(false);
        } else if (ICU::IsLineTerminator(c_)) {
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
  return token;
}

void Lexer::PushBack() {
  if (pos_ < 2) {
    c_ = -1;
  } else {
    c_ = source_->Get(pos_-2);
    --pos_;
  }
}

inline Token::Type Lexer::IsMatch(char const * keyword,
                                    std::size_t len,
                                    Token::Type guess) const {
  std::vector<UChar>::const_iterator it = buffer16_.begin();
  do {
    if (*it++ != *keyword++) {
      return Token::IDENTIFIER;
    }
  } while (--len);
  return guess;
}

Token::Type Lexer::SkipSingleLineComment() {
  Advance();
  // see ECMA-262 section 7.4
  while (c_ >= 0 && !ICU::IsLineTerminator(c_)) {
    Advance();
  }
  return Token::NOT_FOUND;
}

Token::Type Lexer::SkipMultiLineComment() {
  Advance();
  // remember previous ch
  UChar ch;
  while (c_ >= 0) {
    ch = c_;
    Advance();
    if (ch == '*' && c_ == '/') {
      c_ = ' ';
      return Token::NOT_FOUND;
    } else if (ICU::IsLineTerminator(c_)) {
      // see ECMA-262 section 7.4
      SkipLineTerminator();
      has_line_terminator_before_next_ = true;
      ch = '\n';
    }
  }
  return Token::ILLEGAL;
}

Token::Type Lexer::ScanHtmlComment() {
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

Token::Type Lexer::ScanMagicComment() {
  Advance();
  // see ECMA-262 section 7.4
  while (c_ >= 0 && !ICU::IsLineTerminator(c_)) {
    Advance();
  }
  return Token::NOT_FOUND;
}

Token::Type Lexer::ScanIdentifier(LexType type) {
  Token::Type token = Token::IDENTIFIER;
  UChar uc;

  buffer16_.clear();

  if (c_ == '\\') {
    Advance();
    if (c_ != 'u') {
      return Token::ILLEGAL;
    }
    Advance();
    uc = ScanHexEscape('u', 4);
    if (uc == '\\' || !ICU::IsIdentifierStart(uc)) {
      return Token::ILLEGAL;
    }
    Record16(uc);
  } else {
    Record16Advance();
  }

  while (ICU::IsIdentifierPart(c_)) {
    if (c_ == '\\') {
      Advance();
      if (c_ != 'u') {
        return Token::ILLEGAL;
      }
      Advance();
      uc = ScanHexEscape('u', 4);
      if (uc == '\\' || !ICU::IsIdentifierPart(uc)) {
        return Token::ILLEGAL;
      }
      Record16(uc);
    } else {
      Record16Advance();
    }
  }

  if (type == kIdentifyReservedWords) {
    token = DetectKeyword();
  } else if (type == kIgnoreReservedWordsAndIdentifyGetterOrSetter) {
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
Token::Type Lexer::DetectKeyword() const {
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
      // for var int new try
      switch (buffer16_[2]) {
        case 't':
          if (buffer16_[1] == 't' && buffer16_[0] == 'i') {
            // int
            // token = Token::INT;
            token = Token::IDENTIFIER;
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
            // byte
            // token = Token::BYTE;
            token = Token::IDENTIFIER;
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
          // long
          if (buffer16_[0] == 'l' &&
              buffer16_[1] == 'o' && buffer16_[2] == 'n') {
            // token = Token::LONG;
            token = Token::IDENTIFIER;
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
          // char
          if (buffer16_[0] == 'c' &&
              buffer16_[1] == 'h' && buffer16_[2] == 'a') {
            // token = Token::CHAR;
            token = Token::IDENTIFIER;
          }
          break;
        case 'o':
          // goto
          if (buffer16_[0] == 'g' &&
              buffer16_[1] == 'o' && buffer16_[2] == 't') {
            // token = Token::GOTO;
            token = Token::IDENTIFIER;
          }
          break;
      }
      break;
    case 5:
      // break final float catch super while
      // throw short class const false
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
              // final
              // token = Token::FINAL;
              token = Token::IDENTIFIER;
            } else if (buffer16_[1] == 'l' &&
                       buffer16_[2] == 'o' && buffer16_[4] == 't') {
              // float
              // token = Token::FLOAT;
              token = Token::IDENTIFIER;
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
            // short
            // token = Token::SHORT;
            token = Token::IDENTIFIER;
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
            token = Token::IDENTIFIER;
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
          // native
          // token = IsMatch("native", len, Token::NATIVE);
          token = Token::IDENTIFIER;
          break;
        case 'p':
          // public
          token = IsMatch("public", len, Token::PUBLIC);
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
          } else if (buffer16_[1] == 't' &&
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
            // throws
            // token = Token::THROWS;
            token = Token::IDENTIFIER;
          }
          break;
      }
      break;
    case 7:
      // boolean default extends finally package private
      // number 0 character is most duplicated
      switch (buffer16_[0]) {
        case 'b':
          // token = IsMatch("boolean", len, Token::BOOLEAN);
          token = Token::IDENTIFIER;
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
            token = IsMatch("package", len, Token::PACKAGE);
          } else if (buffer16_[1] == 'r') {
            token = IsMatch("private", len, Token::PRIVATE);
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
          // token = IsMatch("abstract", len, Token::ABSTRACT);
          token = Token::IDENTIFIER;
          break;
        case 't':
          if (buffer16_[1] == 'o') {
            // token = IsMatch("volatile", len, Token::VOLATILE);
            token = Token::IDENTIFIER;
          } else if (buffer16_[1] == 'u') {
            token = IsMatch("function", len, Token::FUNCTION);
          }
          break;
      }
      break;
    case 9:
      // interface protected transient
      if (buffer16_[1] == 'n') {
        token = IsMatch("interface", len, Token::INTERFACE);
      } else if (buffer16_[1] == 'r') {
        if (buffer16_[0] == 'p') {
          token = IsMatch("protected", len, Token::PROTECTED);
        } else if (buffer16_[0] == 't') {
          // token = IsMatch("transient", len, Token::TRANSIENT);
          token = Token::IDENTIFIER;
        }
      }
      break;
    case 10:
      // instanceof implements
      if (buffer16_[1] == 'n') {
        token = IsMatch("instanceof", len, Token::INSTANCEOF);
      } else if (buffer16_[1] == 'm') {
        token = IsMatch("implements", len, Token::IMPLEMENTS);
      }
      break;
    case 12:
      // synchronized
      // token = IsMatch("synchronized", len, Token::SYNCHRONIZED);
      token = Token::IDENTIFIER;
      break;
  }
  return token;
}

Token::Type Lexer::DetectGetOrSet() const {
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

Token::Type Lexer::ScanString() {
  type_ = NONE;
  const UChar quote = c_;
  buffer16_.clear();
  Advance();
  while (c_ != quote && c_ >= 0 && !ICU::IsLineTerminator(c_)) {
    if (c_ == '\\') {
      Advance();
      // escape sequence
      if (c_ < 0) return Token::ILLEGAL;
      if (type_ == NONE) {
        type_ = ESCAPE;
      }
      ScanEscape();
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

void Lexer::ScanEscape() {
  if (ICU::IsLineTerminator(c_)) {
    SkipLineTerminator();
    return;
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
    case 'u' :
      Advance();
      Record16(ScanHexEscape('u', 4));
      break;
    case 'v' :
      Record16('\v');
      Advance();
      break;
    case 'x' :
      Advance();
      Record16(ScanHexEscape('x', 2));
      break;
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

    default:
      Record16Advance();
      break;
  }
}

Token::Type Lexer::ScanNumber(const bool period) {
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
        if (!ICU::IsHexDigit(c_)) {
          return Token::ILLEGAL;
        }
        while (ICU::IsHexDigit(c_)) {
          Record8Advance();
        }
      } else if (ICU::IsOctalDigit(c_)) {
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
    if (!ICU::IsDecimalDigit(c_)) {
      return Token::ILLEGAL;
    }
    ScanDecimalDigits();
  }

  // see ECMA-262 section 7.8.3
  // "immediately following a NumericLiteral must not be an IdentifierStart or
  // DecimalDigit."
  if (ICU::IsDecimalDigit(c_) || ICU::IsIdentifierStart(c_)) {
    return Token::ILLEGAL;
  }

  if (type == OCTAL) {
    double val = 0;
    for (std::vector<char>::const_iterator it = buffer8_.begin(),
         last = buffer8_.end(); it != last; ++it) {
      val = val * 8 + (*it - '0');
    }
    numeric_ = val;
  } else {
    Record8('\0');  // Null Terminated String
    numeric_ = std::strtod(buffer8_.data(), NULL);
  }
  type_ = type;
  return Token::NUMBER;
}

UChar Lexer::ScanOctalEscape() {
  UChar res = 0;
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

UChar Lexer::ScanHexEscape(UChar c, int len) {
  UChar res = 0;
  for (int i = 0; i < len; ++i) {
    const int d = HexValue(c_);
    if (d < 0) {
      for (int j = i - 1; j >= 0; --j) {
        PushBack();
      }
      return c;
    }
    res = res * 16 + d;
    Advance();
  }
  return res;
}

inline int Lexer::OctalValue(const int c) const {
  if ('0' <= c && c <= '8') {
    return c - '0';
  }
  return -1;
}

inline int Lexer::HexValue(const int c) const {
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

void Lexer::ScanDecimalDigits() {
  while (ICU::IsDecimalDigit(c_)) {
    Record8Advance();
  }
}

void Lexer::SkipLineTerminator() {
  const UChar c = c_;
  Advance();
  if (c + c_ == '\n' + '\r') {
    Advance();
  }
  ++line_number_;
}

bool Lexer::ScanRegExpLiteral(bool contains_eq) {
  bool character = false;
  buffer16_.clear();
  if (contains_eq) {
    Record16('=');
  }
  while (c_ != '/' || character) {
    // invalid RegExp pattern
    if (ICU::IsLineTerminator(c_) || c_ < 0) {
      return false;
    }
    if (c_ == '\\') {
      // escape
      Record16Advance();
      if (ICU::IsLineTerminator(c_) || c_ < 0) {
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
  Advance();
  return true;
}

bool Lexer::ScanRegExpFlags() {
  buffer16_.clear();
  UChar uc;
  while (ICU::IsIdentifierPart(c_)) {
    if (c_ == '\\') {
      Advance();
      if (c_ != 'u') {
        return false;
      }
      Advance();
      uc = ScanHexEscape('u', 4);
      if (uc == '\\') {
        return false;
      }
      Record16(uc);
    } else {
      Record16Advance();
    }
  }
  return true;
}

} }  // namespace iv::core

