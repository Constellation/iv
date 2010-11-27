#ifndef _IV_KEYWORD_H_
#define _IV_KEYWORD_H_
#include <cstddef>
#include "token.h"
namespace iv {
namespace core {

// tags
class IdentifyReservedWords { };
class IgnoreReservedWords { };
class IgnoreReservedWordsAndIdentifyGetterOrSetter { };

namespace detail {
template<typename LexType>
class Keyword { };

// detect which Identifier is Keyword, FutureReservedWord or not
// Keyword and FutureReservedWord are defined in ECMA-262 5th.
//
// Some words such as :
// int, short, boolean, byte, long, char, float, double, abstract, volatile,
// transient, final, throws, goto, native, synchronized
// were defined as FutureReservedWord in ECMA-262 3rd, but not in 5th.
// So, Detect interprets them as Identifier.
template<>
class Keyword<IdentifyReservedWords> {
 public:
  template<typename Buffer>
  static Token::Type Detect(const Buffer& buf, bool strict) {
    const std::size_t len = buf.size();
    Token::Type token = Token::IDENTIFIER;
    switch (len) {
      case 2:
        // if in do
        if (buf[0] == 'i') {
          if (buf[1] == 'f') {
            token = Token::IF;
          } else if (buf[1] == 'n') {
            token = Token::IN;
          }
        } else if (buf[0] == 'd' && buf[1] == 'o') {
          // do
          token = Token::DO;
        }
        break;
      case 3:
        // for var int new try let
        switch (buf[2]) {
          case 't':
            if (buf[0] == 'l' && buf[1] == 'e' && strict) {
              // let
              token = Token::LET;
            } else if (buf[0] == 'i' && buf[1] == 'n') {
              // int (removed)
              // token = Token::INT;
            }
            break;
          case 'r':
            // for var
            if (buf[0] == 'f' && buf[1] == 'o') {
              // for
              token = Token::FOR;
            } else if (buf[0] == 'v' && buf[1] == 'a') {
              // var
              token = Token::VAR;
            }
            break;
          case 'y':
            // try
            if (buf[0] == 't' && buf[1] == 'r') {
              token = Token::TRY;
            }
            break;
          case 'w':
            // new
            if (buf[0] == 'n' && buf[1] == 'e') {
              token = Token::NEW;
            }
            break;
        }
        break;
      case 4:
        // else case true byte null this
        // void with long enum char goto
        // number 3 character is most duplicated
        switch (buf[3]) {
          case 'e':
            // else case true byte
            if (buf[2] == 's') {
              if (buf[0] == 'e' && buf[1] == 'l') {
                // else
                token = Token::ELSE;
              } else if (buf[0] == 'c' && buf[1] == 'a') {
                // case
                token = Token::CASE;
              }
            } else if (buf[0] == 't' &&
                       buf[1] == 'r' && buf[2] == 'u') {
              // true
              token = Token::TRUE_LITERAL;
            } else if (buf[0] == 'b' &&
                       buf[1] == 'y' && buf[2] == 't') {
              // byte (removed)
              // token = Token::BYTE;
            }
            break;
          case 'l':
            // null
            if (buf[0] == 'n' &&
                buf[1] == 'u' && buf[2] == 'l') {
              token = Token::NULL_LITERAL;
            }
            break;
          case 's':
            // this
            if (buf[0] == 't' &&
                buf[1] == 'h' && buf[2] == 'i') {
              token = Token::THIS;
            }
            break;
          case 'd':
            // void
            if (buf[0] == 'v' &&
                buf[1] == 'o' && buf[2] == 'i') {
              token = Token::VOID;
            }
            break;
          case 'h':
            // with
            if (buf[0] == 'w' &&
                buf[1] == 'i' && buf[2] == 't') {
              token = Token::WITH;
            }
            break;
          case 'g':
            // long (removed)
            if (buf[0] == 'l' &&
                buf[1] == 'o' && buf[2] == 'n') {
              // token = Token::LONG;
            }
            break;
          case 'm':
            // enum
            if (buf[0] == 'e' &&
                buf[1] == 'n' && buf[2] == 'u') {
              token = Token::ENUM;
            }
            break;
          case 'r':
            // char (removed)
            if (buf[0] == 'c' &&
                buf[1] == 'h' && buf[2] == 'a') {
              // token = Token::CHAR;
            }
            break;
          case 'o':
            // goto (removed)
            if (buf[0] == 'g' &&
                buf[1] == 'o' && buf[2] == 't') {
              // token = Token::GOTO;
            }
            break;
        }
        break;
      case 5:
        // break final float catch super while
        // throw short class const false yield
        // number 3 character is most duplicated
        switch (buf[3]) {
          case 'a':
            // break final float
            if (buf[0] == 'b' && buf[1] == 'r' &&
                buf[2] == 'e' && buf[4] == 'k') {
              // break
              token = Token::BREAK;
            } else if (buf[0] == 'f') {
              if (buf[1] == 'i' &&
                  buf[2] == 'n' && buf[4] == 'l') {
                // final (removed)
                // token = Token::FINAL;
              } else if (buf[1] == 'l' &&
                         buf[2] == 'o' && buf[4] == 't') {
                // float (removed)
                // token = Token::FLOAT;
              }
            }
            break;
          case 'c':
            if (buf[0] == 'c' && buf[1] == 'a' &&
                buf[2] == 't' && buf[4] == 'h') {
              // catch
              token = Token::CATCH;
            }
            break;
          case 'e':
            if (buf[0] == 's' && buf[1] == 'u' &&
                buf[2] == 'p' && buf[4] == 'r') {
              // super
              token = Token::SUPER;
            }
            break;
          case 'l':
            if (buf[0] == 'w' && buf[1] == 'h' &&
                buf[2] == 'i' && buf[4] == 'e') {
              // while
              token = Token::WHILE;
            } else if (strict &&
                       buf[0] == 'y' && buf[1] == 'i' &&
                       buf[2] == 'e' && buf[4] == 'd') {
              // yield
              token = Token::YIELD;
            }
            break;
          case 'o':
            if (buf[0] == 't' && buf[1] == 'h' &&
                buf[2] == 'r' && buf[4] == 'w') {
              // throw
              token = Token::THROW;
            }
            break;
          case 'r':
            if (buf[0] == 's' && buf[1] == 'h' &&
                buf[2] == 'o' && buf[4] == 't') {
              // short (removed)
              // token = Token::SHORT;
            }
            break;
          case 's':
            // class const false
            if (buf[0] == 'c') {
              if (buf[1] == 'l' &&
                  buf[2] == 'a' && buf[4] == 's') {
                // class
                token = Token::CLASS;
              } else if (buf[1] == 'o' &&
                         buf[2] == 'n' && buf[4] == 't') {
                // const
                token = Token::CONST;
              }
            } else if (buf[0] == 'f' && buf[1] == 'a' &&
                       buf[2] == 'l' && buf[4] == 'e') {
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
        switch (buf[0]) {
          case 'd':
            // double delete
            if (buf[5] == 'e' &&
                buf[4] == 'l' && buf[3] == 'b' &&
                buf[2] == 'u' && buf[1] == 'o') {
              // double
              // token = Token::DOUBLE;
            } else if (buf[5] == 'e' &&
                       buf[4] == 't' && buf[3] == 'e' &&
                       buf[2] == 'l' && buf[1] == 'e') {
              // delete
              token = Token::DELETE;
            }
            break;
          case 'e':
            // export
            token = IsMatch("export", buf, Token::EXPORT);
            break;
          case 'i':
            // import
            token = IsMatch("import", buf, Token::IMPORT);
            break;
          case 'n':
            // native (removed)
            // token = IsMatch("native", buf, Token::NATIVE);
            break;
          case 'p':
            // public
            token = IsMatch("public", buf, Token::PUBLIC, strict);
            break;
          case 'r':
            // return
            token = IsMatch("return", buf, Token::RETURN);
            break;
          case 's':
            // switch static
            if (buf[1] == 'w' &&
                buf[2] == 'i' && buf[3] == 't' &&
                buf[4] == 'c' && buf[5] == 'h') {
              // switch
              token = Token::SWITCH;
            } else if (strict &&
                       buf[1] == 't' &&
                       buf[2] == 'a' && buf[3] == 't' &&
                       buf[4] == 'i' && buf[5] == 'c') {
              // static
              token = Token::STATIC;
            }
            break;
          case 't':
            // typeof throws
            if (buf[5] == 'f' &&
                buf[4] == 'o' && buf[3] == 'e' &&
                buf[2] == 'p' && buf[1] == 'y') {
              // typeof
              token = Token::TYPEOF;
            } else if (buf[5] == 's' &&
                       buf[4] == 'w' && buf[3] == 'o' &&
                       buf[2] == 'r' && buf[1] == 'h') {
              // throws (removed)
              // token = Token::THROWS;
            }
            break;
        }
        break;
      case 7:
        // boolean default extends finally package private
        // number 0 character is most duplicated
        switch (buf[0]) {
          case 'b':
            // boolean (removed)
            // token = IsMatch("boolean", buf, Token::BOOLEAN);
            break;
          case 'd':
            token = IsMatch("default", buf, Token::DEFAULT);
            break;
          case 'e':
            token = IsMatch("extends", buf, Token::EXTENDS);
            break;
          case 'f':
            token = IsMatch("finally", buf, Token::FINALLY);
            break;
          case 'p':
            if (buf[1] == 'a') {
              token = IsMatch("package", buf, Token::PACKAGE, strict);
            } else if (buf[1] == 'r') {
              token = IsMatch("private", buf, Token::PRIVATE, strict);
            }
            break;
        }
        break;
      case 8:
        // debugger continue abstract volatile function
        // number 4 character is most duplicated
        switch (buf[4]) {
          case 'g':
            token = IsMatch("debugger", buf, Token::DEBUGGER);
            break;
          case 'i':
            token = IsMatch("continue", buf, Token::CONTINUE);
            break;
          case 'r':
            // abstract (removed)
            // token = IsMatch("abstract", buf, Token::ABSTRACT);
            break;
          case 't':
            if (buf[1] == 'o') {
              // token = IsMatch("volatile", buf, Token::VOLATILE);
            } else if (buf[1] == 'u') {
              token = IsMatch("function", buf, Token::FUNCTION);
            }
            break;
        }
        break;
      case 9:
        // interface protected transient
        if (buf[1] == 'n') {
          token = IsMatch("interface", buf, Token::INTERFACE, strict);
        } else if (buf[1] == 'r') {
          if (buf[0] == 'p') {
            token = IsMatch("protected", buf, Token::PROTECTED, strict);
          } else if (buf[0] == 't') {
            // transient (removed)
            // token = IsMatch("transient", buf, Token::TRANSIENT);
          }
        }
        break;
      case 10:
        // instanceof implements
        if (buf[1] == 'n') {
          token = IsMatch("instanceof", buf, Token::INSTANCEOF);
        } else if (buf[1] == 'm') {
          token = IsMatch("implements", buf, Token::IMPLEMENTS, strict);
        }
        break;
      case 12:
        // synchronized (removed)
        // token = IsMatch("synchronized", buf, Token::SYNCHRONIZED);
        token = Token::IDENTIFIER;
        break;
    }
    return token;
  }

  template<typename Buffer>
  static inline Token::Type IsMatch(char const * keyword,
                                    const Buffer& buf,
                                    Token::Type guess,
                                    bool strict) {
    if (!strict) {
      return Token::IDENTIFIER;
    }
    typename Buffer::const_iterator it = buf.begin();
    const typename Buffer::const_iterator last = buf.end();
    do {
      if (*it++ != *keyword++) {
        return Token::IDENTIFIER;
      }
    } while (it != last);
    return guess;
  }

  template<typename Buffer>
  static inline Token::Type IsMatch(char const * keyword,
                                    const Buffer& buf,
                                    Token::Type guess) {
    typename Buffer::const_iterator it = buf.begin();
    const typename Buffer::const_iterator last = buf.end();
    do {
      if (*it++ != *keyword++) {
        return Token::IDENTIFIER;
      }
    } while (it != last);
    return guess;
  }
};

template<>
class Keyword<IgnoreReservedWords> {
 public:
  template<typename Buffer>
  static inline Token::Type Detect(const Buffer& buf, bool strict) {
    return Token::IDENTIFIER;
  }
};

template<>
class Keyword<IgnoreReservedWordsAndIdentifyGetterOrSetter> {
 public:
  template<typename Buffer>
  static inline Token::Type Detect(const Buffer& buf, bool strict) {
    if (buf.size() == 3) {
      if (buf[1] == 'e' && buf[2] == 't') {
        if (buf[0] == 'g') {
          return Token::GET;
        } else if (buf[0] == 's') {
          return Token::SET;
        }
      }
    }
    return Token::IDENTIFIER;
  }
};



} } }  // namespace iv::core::detail
#endif  // _IV_KEYWORD_H_
