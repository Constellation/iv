#ifndef _IV_TOKEN_H_
#define _IV_TOKEN_H_
#include <cstddef>
#include <cassert>
#include "none.h"
namespace iv {
namespace core {
namespace detail {
template<typename T>
struct TokenContents {
  static const char* kContents[];
};
}  // namespace iv::core::detail
typedef detail::TokenContents<None> TokenContents;

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
    ASSIGN_BIT_XOR,  // ^=
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
    IMPORT,          // import
    INT,             // int
    LONG,            // long
    NATIVE,          // native
    SHORT,           // short
    SUPER,           // super
    SYNCHRONIZED,    // synchronized
    THROWS,          // throws
    TRANSIENT,       // transient
    VOLATILE,        // volatile

    GET,             // get
    SET,             // set

    STRICT_FIRST,    // STRICT FIRST
    IMPLEMENTS,      // implements
    LET,             // let
    PRIVATE,         // private
    PUBLIC,          // public
    YIELD,           // yield
    INTERFACE,       // interface
    PACKAGE,         // package
    PROTECTED,       // protected
    STATIC,          // static
    STRICT_LAST,     // STRICT LAST

    NULL_LITERAL,    // NULL   LITERAL
    FALSE_LITERAL,   // FALSE  LITERAL
    TRUE_LITERAL,    // TRUE   LITERAL
    NUMBER,          // NUMBER LITERAL
    STRING,          // STRING LITERAL

    IDENTIFIER,      // IDENTIFIER

    NOT_FOUND,       // NOT_FOUND

    NUM_TOKENS       // number of tokens
  };
  static const std::size_t kMaxSize = 12;  // "synchronized"

  static inline bool IsAssignOp(Token::Type type) {
    return Token::ASSIGN_FIRST < type && type < Token::ASSIGN_LAST;
  }

  static inline bool IsAddedFutureReservedWordInStrictCode(Token::Type type) {
    return Token::STRICT_FIRST < type && type < Token::STRICT_LAST;
  }

  static inline const char* ToString(Token::Type type) {
    assert(0 <= type && type < NUM_TOKENS);
    assert(type != Token::ASSIGN_FIRST &&
           type != Token::ASSIGN_LAST &&
           type != Token::REL_FIRST &&
           type != Token::REL_LAST &&
           type != Token::STRICT_FIRST &&
           type != Token::STRICT_LAST &&
           type != Token::STRING &&
           type != Token::NUMBER &&
           type != Token::IDENTIFIER &&
           type != Token::EOS &&
           type != Token::ILLEGAL &&
           type != Token::NOT_FOUND);
    return TokenContents::kContents[type];
  }
};

namespace detail {
template<typename T>
const char* TokenContents<T>::kContents[Token::NUM_TOKENS] = {
  NULL,            // EOS
  NULL,            // ILLEGAL
  ".",
  ":",
  ";",
  ",",
  "(",
  ")",
  "[",
  "]",
  "{",
  "}",
  "?",
  "==",
  "===",
  "!",
  "!=",
  "!==",
  "++",
  "--",
  "+",
  "-",
  "*",
  "/",
  "%",
  NULL,            // RELATIONAL FIRST
  "<",
  ">",
  "<=",
  ">=",
  "instanceof",
  NULL,            // RELATIONAL LAST
  ">>",
  ">>>",
  "<<",
  "&",
  "|",
  "^",
  "~",
  "&&",
  "||",
  "<!--",
  NULL,            // ASSIGN OP FIRST
  "=",
  "+=",
  "-=",
  "*=",
  "%=",
  "/=",
  ">>=",
  ">>>=",
  "<<=",
  "&=",
  "|=",
  "~=",
  NULL,            // ASSIGN OP LAST
  "delete",
  "typeof",
  "void",
  "break",
  "case",
  "catch",
  "continue",
  "debugger",
  "default",
  "do",
  "else",
  "finaly",
  "for",
  "function",
  "if",
  "in",
  "new",
  "return",
  "switch",
  "this",
  "throw",
  "try",
  "var",
  "while",
  "with",
  "abstract",
  "boolean",
  "byte",
  "char",
  "class",
  "const",
  "double",
  "enum",
  "export",
  "extends",
  "final",
  "float",
  "goto",
  "import",
  "int",
  "long",
  "native",
  "short",
  "super",
  "synchronized",
  "throws",
  "transient",
  "volatile",
  "get",
  "set",
  NULL,
  "implements",
  "let",
  "private",
  "public",
  "yield",
  "interface",
  "package",
  "protected",
  "static",
  NULL,
  "null",
  "false",
  "true",
  NULL,            // NUMBER LITERAL
  NULL,            // STRING LITERAL
  NULL,            // IDENTIFIER
  NULL             // NOT FOUND
};

}  // namespace iv::core::detail
} }  // namespace iv::core
#endif  // _IV_TOKEN_H_
