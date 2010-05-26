#ifndef _IV_AST_VISITOR_H_
#define _IV_AST_VISITOR_H_
#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/schriter.h>

namespace iv {
namespace core {
class AstNode;
class Statement;
class Block;
class FunctionStatement;
class VariableStatement;
class Declaration;
class EmptyStatement;
class IfStatement;
class IterationStatement;
class DoWhileStatement;
class WhileStatement;
class ForStatement;
class ForInStatement;
class ContinueStatement;
class BreakStatement;
class ReturnStatement;
class WithStatement;
class LabelledStatement;
class CaseClause;
class SwitchStatement;
class ThrowStatement;
class TryStatement;
class DebuggerStatement;
class ExpressionStatement;

class Assignment;
class BinaryOperation;
class ConditionalExpression;
class UnaryOperation;
class PostfixExpression;

class StringLiteral;
class NumberLiteral;
class Identifier;
class ThisLiteral;
class NullLiteral;
class TrueLiteral;
class FalseLiteral;
class Undefined;
class RegExpLiteral;
class ArrayLiteral;
class ObjectLiteral;
class FunctionLiteral;

class PropertyAccess;
class FunctionCall;
class ConstructorCall;

class AstVisitor {
 public:
  AstVisitor();
  virtual ~AstVisitor() = 0;
  virtual void Visit(Block* block) = 0;
  virtual void Visit(FunctionStatement* func) = 0;
  virtual void Visit(VariableStatement* var) = 0;
  virtual void Visit(Declaration* decl) = 0;
  virtual void Visit(EmptyStatement* empty) = 0;
  virtual void Visit(IfStatement* ifstmt) = 0;
  virtual void Visit(DoWhileStatement* dowhile) = 0;
  virtual void Visit(WhileStatement* whilestmt) = 0;
  virtual void Visit(ForStatement* forstmt) = 0;
  virtual void Visit(ForInStatement* forstmt) = 0;
  virtual void Visit(ContinueStatement* continuestmt) = 0;
  virtual void Visit(BreakStatement* breakstmt) = 0;
  virtual void Visit(ReturnStatement* returnstmt) = 0;
  virtual void Visit(WithStatement* withstmt) = 0;
  virtual void Visit(LabelledStatement* labelledstmt) = 0;
  virtual void Visit(CaseClause* clause) = 0;
  virtual void Visit(SwitchStatement* switchstmt) = 0;
  virtual void Visit(ThrowStatement* throwstmt) = 0;
  virtual void Visit(TryStatement* trystmt) = 0;
  virtual void Visit(DebuggerStatement* debuggerstmt) = 0;
  virtual void Visit(ExpressionStatement* exprstmt) = 0;

  virtual void Visit(Assignment* assign) = 0;
  virtual void Visit(BinaryOperation* binary) = 0;
  virtual void Visit(ConditionalExpression* cond) = 0;
  virtual void Visit(UnaryOperation* unary) = 0;
  virtual void Visit(PostfixExpression* postfix) = 0;

  virtual void Visit(StringLiteral* literal) = 0;
  virtual void Visit(NumberLiteral* literal) = 0;
  virtual void Visit(Identifier* literal) = 0;
  virtual void Visit(ThisLiteral* literal) = 0;
  virtual void Visit(NullLiteral* literal) = 0;
  virtual void Visit(TrueLiteral* literal) = 0;
  virtual void Visit(FalseLiteral* literal) = 0;
  virtual void Visit(Undefined* literal) = 0;
  virtual void Visit(RegExpLiteral* literal) = 0;
  virtual void Visit(ArrayLiteral* literal) = 0;
  virtual void Visit(ObjectLiteral* literal) = 0;
  virtual void Visit(FunctionLiteral* literal) = 0;

  virtual void Visit(PropertyAccess* prop) = 0;
  virtual void Visit(FunctionCall* call) = 0;
  virtual void Visit(ConstructorCall* call) = 0;
};

class AstSerializer: public AstVisitor {
 public:
  AstSerializer() : out_() { }
  ~AstSerializer() { }
  inline const UnicodeString& Out() {
    return out_;
  }
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
  void Visit(PropertyAccess* prop);
  void Visit(FunctionCall* call);
  void Visit(ConstructorCall* call);

 private:
  void DecodeString(const UnicodeString& ustr);
  UnicodeString out_;
};

} }  // namespace iv::core
#endif  // _IV_AST_VISITOR_H_

