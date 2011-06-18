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
class Keyword;

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
    Token::Type token = Token::TK_IDENTIFIER;
    switch (len) {
      case 2:
        // if in do
        if (buf[0] == 'i') {
          if (buf[1] == 'f') {
            token = Token::TK_IF;
          } else if (buf[1] == 'n') {
            token = Token::TK_IN;
          }
        } else if (buf[0] == 'd' && buf[1] == 'o') {
          // do
          token = Token::TK_DO;
        }
        break;
      case 3:
        // for var int new try let
        switch (buf[2]) {
          case 't':
            if (buf[0] == 'l' && buf[1] == 'e' && strict) {
              // let
              token = Token::TK_LET;
            } else if (buf[0] == 'i' && buf[1] == 'n') {
              // int (removed)
              // token = Token::TK_INT;
            }
            break;
          case 'r':
            // for var
            if (buf[0] == 'f' && buf[1] == 'o') {
              // for
              token = Token::TK_FOR;
            } else if (buf[0] == 'v' && buf[1] == 'a') {
              // var
              token = Token::TK_VAR;
            }
            break;
          case 'y':
            // try
            if (buf[0] == 't' && buf[1] == 'r') {
              token = Token::TK_TRY;
            }
            break;
          case 'w':
            // new
            if (buf[0] == 'n' && buf[1] == 'e') {
              token = Token::TK_NEW;
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
                token = Token::TK_ELSE;
              } else if (buf[0] == 'c' && buf[1] == 'a') {
                // case
                token = Token::TK_CASE;
              }
            } else if (buf[0] == 't' &&
                       buf[1] == 'r' && buf[2] == 'u') {
              // true
              token = Token::TK_TRUE_LITERAL;
            } else if (buf[0] == 'b' &&
                       buf[1] == 'y' && buf[2] == 't') {
              // byte (removed)
              // token = Token::TK_BYTE;
            }
            break;
          case 'l':
            // null
            if (buf[0] == 'n' &&
                buf[1] == 'u' && buf[2] == 'l') {
              token = Token::TK_NULL_LITERAL;
            }
            break;
          case 's':
            // this
            if (buf[0] == 't' &&
                buf[1] == 'h' && buf[2] == 'i') {
              token = Token::TK_THIS;
            }
            break;
          case 'd':
            // void
            if (buf[0] == 'v' &&
                buf[1] == 'o' && buf[2] == 'i') {
              token = Token::TK_VOID;
            }
            break;
          case 'h':
            // with
            if (buf[0] == 'w' &&
                buf[1] == 'i' && buf[2] == 't') {
              token = Token::TK_WITH;
            }
            break;
          case 'g':
            // long (removed)
            if (buf[0] == 'l' &&
                buf[1] == 'o' && buf[2] == 'n') {
              // token = Token::TK_LONG;
            }
            break;
          case 'm':
            // enum
            if (buf[0] == 'e' &&
                buf[1] == 'n' && buf[2] == 'u') {
              token = Token::TK_ENUM;
            }
            break;
          case 'r':
            // char (removed)
            if (buf[0] == 'c' &&
                buf[1] == 'h' && buf[2] == 'a') {
              // token = Token::TK_CHAR;
            }
            break;
          case 'o':
            // goto (removed)
            if (buf[0] == 'g' &&
                buf[1] == 'o' && buf[2] == 't') {
              // token = Token::TK_GOTO;
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
              token = Token::TK_BREAK;
            } else if (buf[0] == 'f') {
              if (buf[1] == 'i' &&
                  buf[2] == 'n' && buf[4] == 'l') {
                // final (removed)
                // token = Token::TK_FINAL;
              } else if (buf[1] == 'l' &&
                         buf[2] == 'o' && buf[4] == 't') {
                // float (removed)
                // token = Token::TK_FLOAT;
              }
            }
            break;
          case 'c':
            if (buf[0] == 'c' && buf[1] == 'a' &&
                buf[2] == 't' && buf[4] == 'h') {
              // catch
              token = Token::TK_CATCH;
            }
            break;
          case 'e':
            if (buf[0] == 's' && buf[1] == 'u' &&
                buf[2] == 'p' && buf[4] == 'r') {
              // super
              token = Token::TK_SUPER;
            }
            break;
          case 'l':
            if (buf[0] == 'w' && buf[1] == 'h' &&
                buf[2] == 'i' && buf[4] == 'e') {
              // while
              token = Token::TK_WHILE;
            } else if (strict &&
                       buf[0] == 'y' && buf[1] == 'i' &&
                       buf[2] == 'e' && buf[4] == 'd') {
              // yield
              token = Token::TK_YIELD;
            }
            break;
          case 'o':
            if (buf[0] == 't' && buf[1] == 'h' &&
                buf[2] == 'r' && buf[4] == 'w') {
              // throw
              token = Token::TK_THROW;
            }
            break;
          case 'r':
            if (buf[0] == 's' && buf[1] == 'h' &&
                buf[2] == 'o' && buf[4] == 't') {
              // short (removed)
              // token = Token::TK_SHORT;
            }
            break;
          case 's':
            // class const false
            if (buf[0] == 'c') {
              if (buf[1] == 'l' &&
                  buf[2] == 'a' && buf[4] == 's') {
                // class
                token = Token::TK_CLASS;
              } else if (buf[1] == 'o' &&
                         buf[2] == 'n' && buf[4] == 't') {
                // const
                token = Token::TK_CONST;
              }
            } else if (buf[0] == 'f' && buf[1] == 'a' &&
                       buf[2] == 'l' && buf[4] == 'e') {
              // false
              token = Token::TK_FALSE_LITERAL;
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
              // token = Token::TK_DOUBLE;
            } else if (buf[5] == 'e' &&
                       buf[4] == 't' && buf[3] == 'e' &&
                       buf[2] == 'l' && buf[1] == 'e') {
              // delete
              token = Token::TK_DELETE;
            }
            break;
          case 'e':
            // export
            token = IsMatch("export", buf, Token::TK_EXPORT);
            break;
          case 'i':
            // import
            token = IsMatch("import", buf, Token::TK_IMPORT);
            break;
          case 'n':
            // native (removed)
            // token = IsMatch("native", buf, Token::TK_NATIVE);
            break;
          case 'p':
            // public
            token = IsMatch("public", buf, Token::TK_PUBLIC, strict);
            break;
          case 'r':
            // return
            token = IsMatch("return", buf, Token::TK_RETURN);
            break;
          case 's':
            // switch static
            if (buf[1] == 'w' &&
                buf[2] == 'i' && buf[3] == 't' &&
                buf[4] == 'c' && buf[5] == 'h') {
              // switch
              token = Token::TK_SWITCH;
            } else if (strict &&
                       buf[1] == 't' &&
                       buf[2] == 'a' && buf[3] == 't' &&
                       buf[4] == 'i' && buf[5] == 'c') {
              // static
              token = Token::TK_STATIC;
            }
            break;
          case 't':
            // typeof throws
            if (buf[5] == 'f' &&
                buf[4] == 'o' && buf[3] == 'e' &&
                buf[2] == 'p' && buf[1] == 'y') {
              // typeof
              token = Token::TK_TYPEOF;
            } else if (buf[5] == 's' &&
                       buf[4] == 'w' && buf[3] == 'o' &&
                       buf[2] == 'r' && buf[1] == 'h') {
              // throws (removed)
              // token = Token::TK_THROWS;
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
            // token = IsMatch("boolean", buf, Token::TK_BOOLEAN);
            break;
          case 'd':
            token = IsMatch("default", buf, Token::TK_DEFAULT);
            break;
          case 'e':
            token = IsMatch("extends", buf, Token::TK_EXTENDS);
            break;
          case 'f':
            token = IsMatch("finally", buf, Token::TK_FINALLY);
            break;
          case 'p':
            if (buf[1] == 'a') {
              token = IsMatch("package", buf, Token::TK_PACKAGE, strict);
            } else if (buf[1] == 'r') {
              token = IsMatch("private", buf, Token::TK_PRIVATE, strict);
            }
            break;
        }
        break;
      case 8:
        // debugger continue abstract volatile function
        // number 4 character is most duplicated
        switch (buf[4]) {
          case 'g':
            token = IsMatch("debugger", buf, Token::TK_DEBUGGER);
            break;
          case 'i':
            token = IsMatch("continue", buf, Token::TK_CONTINUE);
            break;
          case 'r':
            // abstract (removed)
            // token = IsMatch("abstract", buf, Token::TK_ABSTRACT);
            break;
          case 't':
            if (buf[1] == 'o') {
              // token = IsMatch("volatile", buf, Token::TK_VOLATILE);
            } else if (buf[1] == 'u') {
              token = IsMatch("function", buf, Token::TK_FUNCTION);
            }
            break;
        }
        break;
      case 9:
        // interface protected transient
        if (buf[1] == 'n') {
          token = IsMatch("interface", buf, Token::TK_INTERFACE, strict);
        } else if (buf[1] == 'r') {
          if (buf[0] == 'p') {
            token = IsMatch("protected", buf, Token::TK_PROTECTED, strict);
          } else if (buf[0] == 't') {
            // transient (removed)
            // token = IsMatch("transient", buf, Token::TK_TRANSIENT);
          }
        }
        break;
      case 10:
        // instanceof implements
        if (buf[1] == 'n') {
          token = IsMatch("instanceof", buf, Token::TK_INSTANCEOF);
        } else if (buf[1] == 'm') {
          token = IsMatch("implements", buf, Token::TK_IMPLEMENTS, strict);
        }
        break;
      case 12:
        // synchronized (removed)
        // token = IsMatch("synchronized", buf, Token::TK_SYNCHRONIZED);
        token = Token::TK_IDENTIFIER;
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
      return Token::TK_IDENTIFIER;
    }
    typename Buffer::const_iterator it = buf.begin();
    const typename Buffer::const_iterator last = buf.end();
    do {
      if (*it++ != *keyword++) {
        return Token::TK_IDENTIFIER;
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
        return Token::TK_IDENTIFIER;
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
    return Token::TK_IDENTIFIER;
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
          return Token::TK_GET;
        } else if (buf[0] == 's') {
          return Token::TK_SET;
        }
      }
    }
    return Token::TK_IDENTIFIER;
  }
};



} } }  // namespace iv::core::detail
#endif  // _IV_KEYWORD_H_
