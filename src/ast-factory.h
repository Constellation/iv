#ifndef _IV_AST_FACTORY_H_
#define _IV_AST_FACTORY_H_
#include "functor.h"
#include "ast.h"
#include "alloc.h"
#include "ustringpiece.h"

namespace iv {
namespace core {

class BasicAstFactory : public Space {
 public:
  BasicAstFactory()
    : Space(),
      undefined_instance_(new(this)Undefined()),
      empty_statement_instance_(new(this)EmptyStatement()),
      debugger_statement_instance_(new(this)DebuggerStatement()),
      this_instance_(new(this)ThisLiteral()),
      null_instance_(new(this)NullLiteral()),
      true_instance_(new(this)TrueLiteral()),
      false_instance_(new(this)FalseLiteral()) {
  }

  inline void Clear() {
    Space::Clear();
  }

  Identifier* NewIdentifier(const UStringPiece& buffer) {
    return new (this) Identifier(buffer, this);
  }

  Identifier* NewIdentifier(const std::vector<uc16>& buffer) {
    return new (this) Identifier(buffer, this);
  }

  Identifier* NewIdentifier(const std::vector<char>& buffer) {
    return new (this) Identifier(buffer, this);
  }

  NumberLiteral* NewNumberLiteral(const double& val) {
    return new (this) NumberLiteral(val);
  }

  StringLiteral* NewStringLiteral(const std::vector<uc16>& buffer) {
    return new (this) StringLiteral(buffer, this);
  }

  Directivable* NewDirectivable(const std::vector<uc16>& buffer) {
    return new (this) Directivable(buffer, this);
  }

  RegExpLiteral* NewRegExpLiteral(const std::vector<uc16>& content,
                                  const std::vector<uc16>& flags) {
    return new (this) RegExpLiteral(content, flags, this);
  }

  FunctionLiteral* NewFunctionLiteral(FunctionLiteral::DeclType type) {
    return new (this) FunctionLiteral(type, this);
  }

  ArrayLiteral* NewArrayLiteral() {
    return new (this) ArrayLiteral(this);
  }

  ObjectLiteral* NewObjectLiteral() {
    return new (this) ObjectLiteral(this);
  }

  AstNode::Identifiers* NewLabels() {
    typedef AstNode::Identifiers Identifiers;
    return new (New(sizeof(Identifiers)))
        Identifiers(Identifiers::allocator_type(this));
  }

  NullLiteral* NewNullLiteral() {
    return null_instance_;
  }

  EmptyStatement* NewEmptyStatement() {
    return empty_statement_instance_;
  }

  DebuggerStatement* NewDebuggerStatement() {
    return debugger_statement_instance_;
  }

  ThisLiteral* NewThisLiteral() {
    return this_instance_;
  }

  Undefined* NewUndefined() {
    return undefined_instance_;
  }

  TrueLiteral* NewTrueLiteral() {
    return true_instance_;
  }

  FalseLiteral* NewFalseLiteral() {
    return false_instance_;
  }

  FunctionStatement* NewFunctionStatement(FunctionLiteral* func) {
    return new (this) FunctionStatement(func);
  }

  Block* NewBlock() {
    return new (this) Block(this);
  }

  VariableStatement* NewVariableStatement(Token::Type token) {
    return new (this) VariableStatement(token, this);
  }

  Declaration* NewDeclaration(Identifier* name, Expression* expr) {
    return new (this) Declaration(name, expr);
  }

  IfStatement* NewIfStatement(Expression* cond, Statement* then) {
    return new (this) IfStatement(cond, then);
  }

  DoWhileStatement* NewDoWhileStatement() {
    return new (this) DoWhileStatement();
  }

  WhileStatement* NewWhileStatement(Expression* expr) {
    return new (this) WhileStatement(expr);
  }

  ForInStatement* NewForInStatement(Statement* each, Expression* enumerable) {
    return new (this) ForInStatement(each, enumerable);
  }

  ExpressionStatement* NewExpressionStatement(Expression* expr) {
    return new (this) ExpressionStatement(expr);
  }

  ForStatement* NewForStatement() {
    return new (this) ForStatement();
  }

  ContinueStatement* NewContinueStatement() {
    return new (this) ContinueStatement();
  }

  BreakStatement* NewBreakStatement() {
    return new (this) BreakStatement();
  }

  ReturnStatement* NewReturnStatement(Expression* expr) {
    return new (this) ReturnStatement(expr);
  }

  WithStatement* NewWithStatement(Expression* expr, Statement* stmt) {
    return new (this) WithStatement(expr, stmt);
  }

  SwitchStatement* NewSwitchStatement(Expression* expr) {
    return new (this) SwitchStatement(expr, this);
  }

  CaseClause* NewCaseClause() {
    return new (this) CaseClause(this);
  }

  ThrowStatement*  NewThrowStatement(Expression* expr) {
    return new (this) ThrowStatement(expr);
  }

  TryStatement* NewTryStatement(Block* block) {
    return new (this) TryStatement(block);
  }

  LabelledStatement* NewLabelledStatement(Expression* expr, Statement* stmt) {
    return new (this) LabelledStatement(expr, stmt);
  }

  BinaryOperation* NewBinaryOperation(Token::Type op,
                                      Expression* result, Expression* right) {
    return new (this) BinaryOperation(op, result, right);
  }

  template<Token::Type OP>
  BinaryOperation* NewBinaryOperation(Expression* left, Expression* right) {
    return new (this) BinaryOperation(OP, left, right);
  }

  Assignment* NewAssignment(Token::Type op,
                            Expression* left, Expression* right) {
    return new (this) Assignment(op, left, right);
  }

  ConditionalExpression* NewConditionalExpression(Expression* cond,
                                                  Expression* left,
                                                  Expression* right) {
    return new (this) ConditionalExpression(cond, left, right);
  }

  UnaryOperation* NewUnaryOperation(Token::Type op, Expression* expr) {
    return new (this) UnaryOperation(op, expr);
  }

  PostfixExpression* NewPostfixExpression(Token::Type op, Expression* expr) {
    return new (this) PostfixExpression(op, expr);
  }

  FunctionCall* NewFunctionCall(Expression* expr) {
    return new (this) FunctionCall(expr, this);
  }

  ConstructorCall* NewConstructorCall(Expression* target) {
    return new (this) ConstructorCall(target, this);
  }

  IndexAccess* NewIndexAccess(Expression* expr, Expression* index) {
    return new (this) IndexAccess(expr, index);
  }

  IdentifierAccess* NewIdentifierAccess(Expression* expr, Identifier* ident) {
    return new (this) IdentifierAccess(expr, ident);
  }

 private:
  Undefined* undefined_instance_;
  EmptyStatement* empty_statement_instance_;
  DebuggerStatement* debugger_statement_instance_;
  ThisLiteral* this_instance_;
  NullLiteral* null_instance_;
  TrueLiteral* true_instance_;
  FalseLiteral* false_instance_;
};

} }  // namespace iv::core
#endif  // _IV_AST_FACTORY_H_
