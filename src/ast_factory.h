#ifndef _IV_AST_FACTORY_H_
#define _IV_AST_FACTORY_H_
#include <tr1/type_traits>
#include "functor.h"
#include "location.h"
#include "ast.h"
#include "alloc.h"
#include "static_assert.h"
#include "ustringpiece.h"

namespace iv {
namespace core {
namespace ast {

template<typename Factory>
class BasicAstFactory {
 public:
  typedef BasicAstFactory<Factory> this_type;

#define V(AST) typedef typename ast::AST<Factory> AST;
  AST_NODE_LIST(V)
#undef V
#define V(X, XS) typedef typename SpaceVector<Factory, X*>::type XS;
  AST_LIST_LIST(V)
#undef V
#define V(S) typedef typename SpaceUString<Factory>::type S;
  AST_STRING(V)
#undef V

  BasicAstFactory()
    : undefined_instance_(
          new(static_cast<Factory*>(this))Undefined()),
      empty_statement_instance_(
          new(static_cast<Factory*>(this))EmptyStatement()),
      debugger_statement_instance_(
          new(static_cast<Factory*>(this))DebuggerStatement()),
      this_instance_(
          new(static_cast<Factory*>(this))ThisLiteral()),
      null_instance_(
          new(static_cast<Factory*>(this))NullLiteral()),
      true_instance_(
          new(static_cast<Factory*>(this))TrueLiteral()),
      false_instance_(
          new(static_cast<Factory*>(this))FalseLiteral()) {
    typedef std::tr1::is_convertible<Factory, this_type> is_convertible_to_this;
    typedef std::tr1::is_base_of<this_type, Factory> is_base_of_factory;
    IV_STATIC_ASSERT(is_convertible_to_this::value ||
                     is_base_of_factory::value);
  }

  template<typename Range>
  Identifier* NewIdentifier(const Range& range) {
    return new (static_cast<Factory*>(this))
        Identifier(range, static_cast<Factory*>(this));
  }

  NumberLiteral* NewNumberLiteral(const double& val) {
    return new (static_cast<Factory*>(this)) NumberLiteral(val);
  }

  StringLiteral* NewStringLiteral(const std::vector<uc16>& buffer) {
    return new (static_cast<Factory*>(this))
        StringLiteral(buffer, static_cast<Factory*>(this));
  }

  Directivable* NewDirectivable(const std::vector<uc16>& buffer) {
    return new (static_cast<Factory*>(this))
        Directivable(buffer, static_cast<Factory*>(this));
  }

  RegExpLiteral* NewRegExpLiteral(const std::vector<uc16>& content,
                                  const std::vector<uc16>& flags) {
    return new (static_cast<Factory*>(this))
        RegExpLiteral(content, flags, static_cast<Factory*>(this));
  }

  FunctionLiteral* NewFunctionLiteral(typename FunctionLiteral::DeclType type) {
    return new (static_cast<Factory*>(this))
        FunctionLiteral(type, static_cast<Factory*>(this));
  }

  ArrayLiteral* NewArrayLiteral(Expressions* items) {
    return new (static_cast<Factory*>(this)) ArrayLiteral(items);
  }

  ObjectLiteral*
      NewObjectLiteral(typename ObjectLiteral::Properties* properties) {
    return new (static_cast<Factory*>(this)) ObjectLiteral(properties);
  }

  template<typename T>
  T** NewPtr() {
    return new (static_cast<Factory*>(this)->New(sizeof(T*))) T*(NULL);  // NOLINT
  }

  template<typename T>
  typename SpaceVector<Factory, T>::type* NewVector() {
    typedef typename SpaceVector<Factory, T>::type Vector;
    return new (static_cast<Factory*>(this)->New(sizeof(Vector)))
        Vector(typename Vector::allocator_type(static_cast<Factory*>(this)));
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
    return new (static_cast<Factory*>(this)) FunctionStatement(func);
  }

  FunctionDeclaration* NewFunctionDeclaration(FunctionLiteral* func) {
    return new (static_cast<Factory*>(this)) FunctionDeclaration(func);
  }

  Block* NewBlock(Statements* body) {
    return new (static_cast<Factory*>(this)) Block(body);
  }

  VariableStatement* NewVariableStatement(Token::Type token,
                                          Declarations* decls) {
    return new (static_cast<Factory*>(this))
        VariableStatement(token, decls);
  }

  Declaration* NewDeclaration(Identifier* name, Expression* expr) {
    return new (static_cast<Factory*>(this)) Declaration(name, expr);
  }

  IfStatement* NewIfStatement(Expression* cond,
                              Statement* then_statement,
                              Statement* else_statement) {
    return new (static_cast<Factory*>(this)) IfStatement(cond,
                                                         then_statement,
                                                         else_statement);
  }

  DoWhileStatement* NewDoWhileStatement(Statement* body,
                                        Expression* cond) {
    return new (static_cast<Factory*>(this)) DoWhileStatement(body, cond);
  }

  WhileStatement* NewWhileStatement(Statement* body,
                                    Expression* cond) {
    return new (static_cast<Factory*>(this)) WhileStatement(body, cond);
  }

  ForInStatement* NewForInStatement(Statement* body,
                                    Statement* each,
                                    Expression* enumerable) {
    return new (static_cast<Factory*>(this)) ForInStatement(body,
                                                            each, enumerable);
  }

  ExpressionStatement* NewExpressionStatement(Expression* expr) {
    return new (static_cast<Factory*>(this)) ExpressionStatement(expr);
  }

  ForStatement* NewForStatement(Statement* body,
                                Statement* init,
                                Expression* cond,
                                Statement* next) {
    return new (static_cast<Factory*>(this)) ForStatement(body, init,
                                                          cond, next);
  }

  ContinueStatement* NewContinueStatement(Identifier* label,
                                          IterationStatement** target) {
    return new (static_cast<Factory*>(this)) ContinueStatement(label, target);
  }

  BreakStatement* NewBreakStatement(Identifier* label,
                                    BreakableStatement** target) {
    return new (static_cast<Factory*>(this)) BreakStatement(label, target);
  }

  ReturnStatement* NewReturnStatement(
      Expression* expr) {
    return new (static_cast<Factory*>(this)) ReturnStatement(expr);
  }

  WithStatement* NewWithStatement(
      Expression* expr, Statement* stmt) {
    return new (static_cast<Factory*>(this)) WithStatement(expr, stmt);
  }

  SwitchStatement* NewSwitchStatement(Expression* expr, CaseClauses* clauses) {
    return new (static_cast<Factory*>(this)) SwitchStatement(expr, clauses);
  }

  CaseClause* NewCaseClause(bool is_default,
                            Expression* expr, Statements* body) {
    return new (static_cast<Factory*>(this)) CaseClause(is_default, expr, body);
  }

  ThrowStatement*  NewThrowStatement(Expression* expr) {
    return new (static_cast<Factory*>(this)) ThrowStatement(expr);
  }

  TryStatement* NewTryStatement(Block* try_block,
                                Identifier* catch_name,
                                Block* catch_block,
                                Block* finally_block) {
    return new (static_cast<Factory*>(this)) TryStatement(try_block,
                                                          catch_name,
                                                          catch_block,
                                                          finally_block);
  }

  LabelledStatement* NewLabelledStatement(
      Expression* expr,
      Statement* stmt) {
    return new (static_cast<Factory*>(this)) LabelledStatement(expr, stmt);
  }

  BinaryOperation* NewBinaryOperation(Token::Type op,
                                      Expression* result, Expression* right) {
    return new (static_cast<Factory*>(this)) BinaryOperation(op, result, right);
  }

  Assignment* NewAssignment(Token::Type op,
                            Expression* left,
                            Expression* right) {
    return new (static_cast<Factory*>(this)) Assignment(op, left, right);
  }

  ConditionalExpression* NewConditionalExpression(Expression* cond,
                                                  Expression* left,
                                                  Expression* right) {
    return new (static_cast<Factory*>(this))
        ConditionalExpression(cond, left, right);
  }

  UnaryOperation* NewUnaryOperation(Token::Type op, Expression* expr) {
    return new (static_cast<Factory*>(this)) UnaryOperation(op, expr);
  }

  PostfixExpression* NewPostfixExpression(
      Token::Type op, Expression* expr) {
    return new (static_cast<Factory*>(this)) PostfixExpression(op, expr);
  }

  FunctionCall* NewFunctionCall(Expression* expr, Expressions* args) {
    return new (static_cast<Factory*>(this)) FunctionCall(expr, args);
  }

  ConstructorCall* NewConstructorCall(Expression* target, Expressions* args) {
    return new (static_cast<Factory*>(this)) ConstructorCall(target, args);
  }

  IndexAccess* NewIndexAccess(Expression* expr,
                              Expression* index) {
    return new (static_cast<Factory*>(this)) IndexAccess(expr, index);
  }

  IdentifierAccess* NewIdentifierAccess(Expression* expr,
                                        Identifier* ident) {
    return new (static_cast<Factory*>(this)) IdentifierAccess(expr, ident);
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

} } }  // namespace iv::core::ast
#endif  // _IV_AST_FACTORY_H_
