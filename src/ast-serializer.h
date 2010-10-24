#ifndef _IV_AST_SERIALIZER_H_
#define _IV_AST_SERIALIZER_H_
#include <ostream>  // NOLINT
#include "uchar.h"
#include "ast-visitor.h"
#include "ustring.h"
#include "stringpiece.h"
#include "ustringpiece.h"
namespace iv {
namespace core {

class AstSerializer: public ConstAstVisitor {
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
  void Visit(const Block* block);
  void Visit(const FunctionStatement* func);
  void Visit(const VariableStatement* var);
  void Visit(const Declaration* decl);
  void Visit(const EmptyStatement* empty);
  void Visit(const IfStatement* ifstmt);
  void Visit(const DoWhileStatement* dowhile);
  void Visit(const WhileStatement* whilestmt);
  void Visit(const ForStatement* forstmt);
  void Visit(const ForInStatement* forstmt);
  void Visit(const ContinueStatement* continuestmt);
  void Visit(const BreakStatement* breakstmt);
  void Visit(const ReturnStatement* returnstmt);
  void Visit(const WithStatement* withstmt);
  void Visit(const LabelledStatement* labelledstmt);
  void Visit(const CaseClause* clause);
  void Visit(const SwitchStatement* switchstmt);
  void Visit(const ThrowStatement* throwstmt);
  void Visit(const TryStatement* trystmt);
  void Visit(const DebuggerStatement* debuggerstmt);
  void Visit(const ExpressionStatement* exprstmt);
  void Visit(const Assignment* assign);
  void Visit(const BinaryOperation* binary);
  void Visit(const ConditionalExpression* cond);
  void Visit(const UnaryOperation* unary);
  void Visit(const PostfixExpression* postfix);
  void Visit(const StringLiteral* literal);
  void Visit(const NumberLiteral* literal);
  void Visit(const Identifier* literal);
  void Visit(const ThisLiteral* literal);
  void Visit(const NullLiteral* literal);
  void Visit(const TrueLiteral* literal);
  void Visit(const FalseLiteral* literal);
  void Visit(const Undefined* literal);
  void Visit(const RegExpLiteral* literal);
  void Visit(const ArrayLiteral* literal);
  void Visit(const ObjectLiteral* literal);
  void Visit(const FunctionLiteral* literal);
  void Visit(const IdentifierAccess* prop);
  void Visit(const IndexAccess* prop);
  void Visit(const FunctionCall* call);
  void Visit(const ConstructorCall* call);

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
