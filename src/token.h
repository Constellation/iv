#ifndef _IV_TOKEN_H_
#define _IV_TOKEN_H_
#include <cassert>
#include <unicode/uchar.h>
#include <unicode/unistr.h>

namespace iv {
namespace core {

class Token {
 public:
  enum Type {
    EOS,             // EOS
    ILLEGAL,         // ILLEGAL

    PERIOD,          // .
    COLON,           // :
    SEMICOLON,       // ;
    COMMA,           // ,

    LPAREN,          // (
    RPAREN,          // )
    LBRACK,          // [
    RBRACK,          // ]
    LBRACE,          // {
    RBRACE,          // }

    CONDITIONAL,     // ?

    EQ,              // ==
    EQ_STRICT,       // ===

    NOT,             // !
    NE,              // !=
    NE_STRICT,       // !==

    INC,             // ++
    DEC,             // --

    ADD,             // +
    SUB,             // -
    MUL,             // *
    DIV,             // /
    MOD,             // %

    REL_FIRST,       // RELATIONAL FIRST
    LT,              // <
    GT,              // >
    LTE,             // <=
    GTE,             // >=
    INSTANCEOF,      // instanceof
    REL_LAST,        // RELATIONAL LAST

    SAR,             // >>
    SHR,             // >>>
    SHL,             // <<

    BIT_AND,         // &
    BIT_OR,          // |
    BIT_XOR,         // ^
    BIT_NOT,         // ~

    LOGICAL_AND,     // &&
    LOGICAL_OR,      // ||

    HTML_COMMENT,    // <!--

    ASSIGN_FIRST,    // ASSIGN OP FIRST
    ASSIGN,          // =
    ASSIGN_ADD,      // +=
    ASSIGN_SUB,      // -=
    ASSIGN_MUL,      // *=
    ASSIGN_MOD,      // %=
    ASSIGN_DIV,      // /=
    ASSIGN_SAR,      // >>=
    ASSIGN_SHR,      // >>>=
    ASSIGN_SHL,      // <<=
    ASSIGN_BIT_AND,  // &=
    ASSIGN_BIT_OR,   // |=
    ASSIGN_LAST,     // ASSIGN OP LAST

    DELETE,          // delete
    TYPEOF,          // typeof
    VOID,            // void
    BREAK,           // break
    CASE,            // case
    CATCH,           // catch
    CONTINUE,        // continue
    DEBUGGER,        // debugger
    DEFAULT,         // default
    DO,              // do
    ELSE,            // else
    FINALLY,         // finaly
    FOR,             // for
    FUNCTION,        // function
    IF,              // if
    IN,              // in
    NEW,             // new
    RETURN,          // return
    SWITCH,          // switch
    THIS,            // this
    THROW,           // throw
    TRY,             // try
    VAR,             // var
    WHILE,           // while
    WITH,            // with

    ABSTRACT,        // abstract
    BOOLEAN,         // boolean
    BYTE,            // byte
    CHAR,            // char
    CLASS,           // class
    CONST,           // const
    DOUBLE,          // double
    ENUM,            // enum
    EXPORT,          // export
    EXTENDS,         // extends
    FINAL,           // final
    FLOAT,           // float
    GOTO,            // goto
    IMPLEMENTS,      // implements
    IMPORT,          // import
    INT,             // int
    INTERFACE,       // interface
    LONG,            // long
    NATIVE,          // native
    PACKAGE,         // package
    PRIVATE,         // private
    PROTECTED,       // protected
    PUBLIC,          // public
    SHORT,           // short
    STATIC,          // static
    SUPER,           // super
    SYNCHRONIZED,    // synchronized
    THROWS,          // throws
    TRANSIENT,       // transient
    VOLATILE,        // volatile

    NULL_LITERAL,    // NULL   LITERAL
    FALSE_LITERAL,   // FALSE  LITERAL
    TRUE_LITERAL,    // TRUE   LITERAL
    NUMBER,          // NUMBER LITERAL
    STRING,          // STRING LITERAL

    IDENTIFIER,      // IDENTIFIER

    NOT_FOUND,       // NOT_FOUND

    NUM_TOKENS       // number of tokens
  };
  static const char* kContents[];

  explicit Token(const UChar* ustr, Type t) : type_(t), value_(ustr) {}
  inline Type type() const { return type_; }
  inline const UnicodeString& value() const { return value_; }
  static inline bool IsAssignOp(Token::Type type) {
    return Token::ASSIGN_FIRST < type && type < Token::ASSIGN_LAST;
  }
  static inline const char* Content(Token::Type type) {
    assert(0 <= type && type < NUM_TOKENS);
    return kContents[type];
  }

 private:
  Type type_;
  UnicodeString value_;
};

} }  // namespace iv::core

#endif  // _IV_TOKEN_H_

