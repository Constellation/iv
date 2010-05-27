#ifndef _IV_CODEGEN_H_
#define _IV_CODEGEN_H_
#include "ast.h"
namespace iv {
namespace core {

class CodeGenerator : public AstVisitor {
 public:
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
};

} }  // namespace iv::core
#endif  // _IV_CODEGEN_H_

