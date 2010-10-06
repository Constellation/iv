#ifndef _IV_AST_SERIALIZER_H_
#define _IV_AST_SERIALIZER_H_
#include <ostream>  // NOLINT
#include <unicode/uchar.h>
#include "ast-visitor.h"
#include "ustring.h"
#include "stringpiece.h"
#include "ustringpiece.h"
namespace iv {
namespace core {

class AstSerializer: public AstVisitor {
 public:

  friend std::ostream& operator<<(std::ostream& os, const AstSerializer& out);

  typedef void ReturnType;

  AstSerializer() : out_() { }
  inline const UString& out() const {
    return out_;
  }
  void Append(const StringPiece& str);
  void Append(const UStringPiece& str);
  void Append(UChar c);
  void Append(char c);
  void Visit(Block* block);
  void Visit(FunctionStatement* func);
  void Visit(VariableStatement* var);
  void Visit(Declaration* decl);
  void Visit(EmptyStatement* empty);
  void Visit(IfStatement* ifstmt);
  void Visit(DoWhileStatement* dowhile);
  void Visit(WhileStatement* whilestmt);
  void Visit(ForStatement* forstmt);
  void Visit(ForInStatement* forstmt);
  void Visit(ContinueStatement* continuestmt);
  void Visit(BreakStatement* breakstmt);
  void Visit(ReturnStatement* returnstmt);
  void Visit(WithStatement* withstmt);
  void Visit(LabelledStatement* labelledstmt);
  void Visit(CaseClause* clause);
  void Visit(SwitchStatement* switchstmt);
  void Visit(ThrowStatement* throwstmt);
  void Visit(TryStatement* trystmt);
  void Visit(DebuggerStatement* debuggerstmt);
  void Visit(ExpressionStatement* exprstmt);
  void Visit(Assignment* assign);
  void Visit(BinaryOperation* binary);
  void Visit(ConditionalExpression* cond);
  void Visit(UnaryOperation* unary);
  void Visit(PostfixExpression* postfix);
  void Visit(StringLiteral* literal);
  void Visit(NumberLiteral* literal);
  void Visit(Identifier* literal);
  void Visit(ThisLiteral* literal);
  void Visit(NullLiteral* literal);
  void Visit(TrueLiteral* literal);
  void Visit(FalseLiteral* literal);
  void Visit(Undefined* literal);
  void Visit(RegExpLiteral* literal);
  void Visit(ArrayLiteral* literal);
  void Visit(ObjectLiteral* literal);
  void Visit(FunctionLiteral* literal);
  void Visit(IdentifierAccess* prop);
  void Visit(IndexAccess* prop);
  void Visit(FunctionCall* call);
  void Visit(ConstructorCall* call);

 private:
  template <class Iter>
  void DecodeString(Iter it, const Iter last) {
    char buf[5];
    for (;it != last; ++it) {
      const UChar val = *it;
      switch (val) {
        case '"':
          Append("\\\"");
          break;

        case '\\':
          Append("\\\\");
          break;

        case '/':
          Append("\\/");
          break;

        case '\b':
          Append("\\b");
          break;

        case '\f':
          Append("\\f");
          break;

        case '\n':
          Append("\\n");
          break;

        case '\r':
          Append("\\r");
          break;

        case '\t':
          Append("\\t");
          break;

        case '\x0B':  // \v
          Append("\\u000b");
          break;

        default:
          if (val < 0x20) {
            if (val < 0x10) {
              Append("\\u000");
              std::snprintf(buf, sizeof(buf), "%x", val);
              Append(buf);
            } else if (0x10 <= val && val < 0x20) {
              Append("\\u00");
              std::snprintf(buf, sizeof(buf), "%x", val);
              Append(buf);
            }
          } else if (0x80 <= val) {
            if (0x80 <= val && val < 0x1000) {
              Append("\\u0");
              std::snprintf(buf, sizeof(buf), "%x", val);
              Append(buf);
            } else if (0x1000 <= val) {
              Append("\\u");
              std::snprintf(buf, sizeof(buf), "%x", val);
              Append(buf);
            }
          } else {
            Append(val);
          }
          break;
      }
    }
  }
  UString out_;
};

} }  // namespace iv::core
#endif  // _IV_AST_SERIALIZER_H_
