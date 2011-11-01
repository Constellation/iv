#ifndef IV_TOKEN_H_
#define IV_TOKEN_H_
#include <cstddef>
#include <cassert>
#include <iv/none.h>
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
    TK_EOS = 0,             // EOS
    TK_ILLEGAL,         // ILLEGAL

    TK_PERIOD,          // .
    TK_COLON,           // :
    TK_SEMICOLON,       // ;
    TK_COMMA,           // ,

    TK_LPAREN,          // (
    TK_RPAREN,          // )
    TK_LBRACK,          // [
    TK_RBRACK,          // ]
    TK_LBRACE,          // {
    TK_RBRACE,          // }

    TK_CONDITIONAL,     // ?

    TK_EQ,              // ==
    TK_EQ_STRICT,       // ===

    TK_NOT,             // !
    TK_NE,              // !=
    TK_NE_STRICT,       // !==

    TK_INC,             // ++
    TK_DEC,             // --

    TK_ADD,             // +
    TK_SUB,             // -
    TK_MUL,             // *
    TK_DIV,             // /
    TK_MOD,             // %

    TK_REL_FIRST,       // RELATIONAL FIRST
    TK_LT,              // <
    TK_GT,              // >
    TK_LTE,             // <=
    TK_GTE,             // >=
    TK_INSTANCEOF,      // instanceof
    TK_REL_LAST,        // RELATIONAL LAST

    TK_SAR,             // >>
    TK_SHR,             // >>>
    TK_SHL,             // <<

    TK_BIT_AND,         // &
    TK_BIT_OR,          // |
    TK_BIT_XOR,         // ^
    TK_BIT_NOT,         // ~

    TK_LOGICAL_AND,     // &&
    TK_LOGICAL_OR,      // ||

    TK_HTML_COMMENT,    // <!--

    TK_ASSIGN_FIRST,    // ASSIGN OP FIRST
    TK_ASSIGN,          // =
    TK_ASSIGN_ADD,      // +=
    TK_ASSIGN_SUB,      // -=
    TK_ASSIGN_MUL,      // *=
    TK_ASSIGN_MOD,      // %=
    TK_ASSIGN_DIV,      // /=
    TK_ASSIGN_SAR,      // >>=
    TK_ASSIGN_SHR,      // >>>=
    TK_ASSIGN_SHL,      // <<=
    TK_ASSIGN_BIT_AND,  // &=
    TK_ASSIGN_BIT_OR,   // |=
    TK_ASSIGN_BIT_XOR,  // ^=
    TK_ASSIGN_LAST,     // ASSIGN OP LAST

    TK_DELETE,          // delete
    TK_TYPEOF,          // typeof
    TK_VOID,            // void
    TK_BREAK,           // break
    TK_CASE,            // case
    TK_CATCH,           // catch
    TK_CONTINUE,        // continue
    TK_DEBUGGER,        // debugger
    TK_DEFAULT,         // default
    TK_DO,              // do
    TK_ELSE,            // else
    TK_FINALLY,         // finaly
    TK_FOR,             // for
    TK_FUNCTION,        // function
    TK_IF,              // if
    TK_IN,              // in
    TK_NEW,             // new
    TK_RETURN,          // return
    TK_SWITCH,          // switch
    TK_THIS,            // this
    TK_THROW,           // throw
    TK_TRY,             // try
    TK_VAR,             // var
    TK_WHILE,           // while
    TK_WITH,            // with

    TK_ABSTRACT,        // abstract
    TK_BOOLEAN,         // boolean
    TK_BYTE,            // byte
    TK_CHAR,            // char
    TK_CLASS,           // class
    TK_CONST,           // const
    TK_DOUBLE,          // double
    TK_ENUM,            // enum
    TK_EXPORT,          // export
    TK_EXTENDS,         // extends
    TK_FINAL,           // final
    TK_FLOAT,           // float
    TK_GOTO,            // goto
    TK_IMPORT,          // import
    TK_INT,             // int
    TK_LONG,            // long
    TK_NATIVE,          // native
    TK_SHORT,           // short
    TK_SUPER,           // super
    TK_SYNCHRONIZED,    // synchronized
    TK_THROWS,          // throws
    TK_TRANSIENT,       // transient
    TK_VOLATILE,        // volatile

    TK_GET,             // get
    TK_SET,             // set

    TK_STRICT_FIRST,    // STRICT FIRST
    TK_IMPLEMENTS,      // implements
    TK_LET,             // let
    TK_PRIVATE,         // private
    TK_PUBLIC,          // public
    TK_YIELD,           // yield
    TK_INTERFACE,       // interface
    TK_PACKAGE,         // package
    TK_PROTECTED,       // protected
    TK_STATIC,          // static
    TK_STRICT_LAST,     // STRICT LAST

    TK_NULL_LITERAL,    // NULL   LITERAL
    TK_FALSE_LITERAL,   // FALSE  LITERAL
    TK_TRUE_LITERAL,    // TRUE   LITERAL
    TK_NUMBER,          // NUMBER LITERAL
    TK_STRING,          // STRING LITERAL

    TK_IDENTIFIER,      // IDENTIFIER

    TK_SINGLE_LINE_COMMENT,  // SINGLE LINE COMMENT
    TK_MULTI_LINE_COMMENT,  // MULTI LINE COMMENT

    TK_NOT_FOUND,       // NOT_FOUND

    TK_NUM_TOKENS       // number of tokens
  };
  static const std::size_t kMaxSize = 12;  // "synchronized"

  static inline bool IsAssignOp(Token::Type type) {
    return Token::TK_ASSIGN_FIRST < type && type < Token::TK_ASSIGN_LAST;
  }

  static inline bool IsAddedFutureReservedWordInStrictCode(Token::Type type) {
    return Token::TK_STRICT_FIRST < type && type < Token::TK_STRICT_LAST;
  }

  static inline const char* ToString(Token::Type type) {
    assert(0 <= type && type < TK_NUM_TOKENS);
    assert(type != Token::TK_ASSIGN_FIRST &&
           type != Token::TK_ASSIGN_LAST &&
           type != Token::TK_REL_FIRST &&
           type != Token::TK_REL_LAST &&
           type != Token::TK_STRICT_FIRST &&
           type != Token::TK_STRICT_LAST &&
           type != Token::TK_STRING &&
           type != Token::TK_NUMBER &&
           type != Token::TK_IDENTIFIER &&
           type != Token::TK_EOS &&
           type != Token::TK_ILLEGAL &&
           type != Token::TK_SINGLE_LINE_COMMENT &&
           type != Token::TK_MULTI_LINE_COMMENT &&
           type != Token::TK_NOT_FOUND);
    return TokenContents::kContents[type];
  }
};

namespace detail {
template<typename T>
const char* TokenContents<T>::kContents[Token::TK_NUM_TOKENS] = {
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
  NULL,            // SINGLE LINE COMMENT
  NULL,            // MULTI LINE COMMENT
  NULL             // NOT FOUND
};

}  // namespace iv::core::detail
} }  // namespace iv::core
#endif  // IV_TOKEN_H_
