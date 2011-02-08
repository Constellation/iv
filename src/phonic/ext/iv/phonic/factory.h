#ifndef _IV_PHONIC_FACTORY_H_
#define _IV_PHONIC_FACTORY_H_
#include <tr1/array>
extern "C" {
#include <ruby.h>
#include <ruby/oniguruma.h>
}
#include <iv/alloc.h>
#include <iv/ustringpiece.h>
#include <iv/space.h>
#include "encoding.h"
#include "ast_fwd.h"
namespace iv {
namespace phonic {

class AstFactory : public core::Space<2> {
 public:
  AstFactory()
    : core::Space<2>(),
      undefined_instance_(new(this)Undefined()),
      empty_statement_instance_(new(this)EmptyStatement()),
      debugger_statement_instance_(new(this)DebuggerStatement()),
      this_instance_(new(this)ThisLiteral()),
      null_instance_(new(this)NullLiteral()),
      true_instance_(new(this)TrueLiteral()),
      false_instance_(new(this)FalseLiteral()) {
  }

  template<typename Range>
  Identifier* NewIdentifier(const Range& range) {
    Identifier* ident = new(this)Identifier(range, this);
    return ident;
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
//    OnigErrorInfo einfo;
//    regex_t* reg;
//    // TODO(Constellation) Little Endian?
//    int r = onig_new(&reg,
//                     reinterpret_cast<const OnigUChar*>(content.data()),
//                     reinterpret_cast<const OnigUChar*>(
//                         content.data()+content.size()),
//                     ONIG_OPTION_DEFAULT,
//                     rb_enc_get(Encoding::UTF16LEEncoding()),
//                     ONIG_SYNTAX_DEFAULT, &einfo);
//    if (r != ONIG_NORMAL) {
//      return NULL;
//    }
//    onig_free(reg);
    return new (this) RegExpLiteral(content, flags, this);
  }

  FunctionLiteral* NewFunctionLiteral(FunctionLiteral::DeclType type) {
    return new (this) FunctionLiteral(type, this);
  }

  ArrayLiteral* NewArrayLiteral(Expressions* items) {
    return new (this) ArrayLiteral(items);
  }

  ObjectLiteral* NewObjectLiteral(ObjectLiteral::Properties* properties) {
    return new (this) ObjectLiteral(properties);
  }

  template<typename T>
  T** NewPtr() {
    return new (New(sizeof(T*))) T*(NULL);  // NOLINT
  }

  template<typename T>
  typename core::SpaceVector<AstFactory, T>::type* NewVector() {
    typedef typename core::SpaceVector<AstFactory, T>::type Vector;
    return new (New(sizeof(Vector))) Vector(typename Vector::allocator_type(this));
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

  FunctionDeclaration* NewFunctionDeclaration(FunctionLiteral* func) {
    return new (this) FunctionDeclaration(func);
  }

  Block* NewBlock(Statements* body) {
    return new (this) Block(body);
  }

  VariableStatement* NewVariableStatement(core::Token::Type token,
                                          Declarations* decls) {
    return new (this) VariableStatement(token, decls);
  }

  Declaration* NewDeclaration(Identifier* name, Expression* expr) {
    return new (this) Declaration(name, expr);
  }

  IfStatement* NewIfStatement(Expression* cond,
                              Statement* then_statement,
                              Statement* else_statement) {
    return new (this) IfStatement(cond,
                                  then_statement,
                                  else_statement);
  }

  DoWhileStatement* NewDoWhileStatement(Statement* body,
                                        Expression* cond) {
    return new (this) DoWhileStatement(body, cond);
  }

  WhileStatement* NewWhileStatement(Statement* body,
                                    Expression* cond) {
    return new (this) WhileStatement(body, cond);
  }

  ForInStatement* NewForInStatement(Statement* body,
                                    Statement* each,
                                    Expression* enumerable) {
    return new (this) ForInStatement(body, each, enumerable);
  }

  ExpressionStatement* NewExpressionStatement(Expression* expr) {
    return new (this) ExpressionStatement(expr);
  }

  ForStatement* NewForStatement(Statement* body,
                                Statement* init,
                                Expression* cond,
                                Statement* next) {
    return new (this) ForStatement(body, init, cond, next);
  }


  ContinueStatement* NewContinueStatement(Identifier* label,
                                          IterationStatement** target) {
    return new (this) ContinueStatement(label, target);
  }

  BreakStatement* NewBreakStatement(Identifier* label,
                                    BreakableStatement** target) {
    return new (this) BreakStatement(label, target);
  }

  ReturnStatement* NewReturnStatement(Expression* expr) {
    return new (this) ReturnStatement(expr);
  }

  WithStatement* NewWithStatement(Expression* expr, Statement* stmt) {
    return new (this) WithStatement(expr, stmt);
  }

  SwitchStatement* NewSwitchStatement(Expression* expr, CaseClauses* clauses) {
    return new (this) SwitchStatement(expr, clauses);
  }

  CaseClause* NewCaseClause(bool is_default,
                            Expression* expr, Statements* body) {
    return new (this) CaseClause(is_default, expr, body);
  }


  ThrowStatement*  NewThrowStatement(Expression* expr) {
    return new (this) ThrowStatement(expr);
  }

  TryStatement* NewTryStatement(Block* try_block,
                                Identifier* catch_name,
                                Block* catch_block,
                                Block* finally_block) {
    return new (this) TryStatement(try_block,
                                   catch_name,
                                   catch_block,
                                   finally_block);
  }

  LabelledStatement* NewLabelledStatement(Expression* expr, Statement* stmt) {
    return new (this) LabelledStatement(expr, stmt);
  }

  BinaryOperation* NewBinaryOperation(core::Token::Type op,
                                      Expression* result,
                                      Expression* right) {
    return new (this) BinaryOperation(op, result, right);
  }

  Assignment* NewAssignment(core::Token::Type op,
                            Expression* left,
                            Expression* right) {
    return new (this) Assignment(op, left, right);
  }

  ConditionalExpression* NewConditionalExpression(Expression* cond,
                                                  Expression* left,
                                                  Expression* right) {
    return new (this) ConditionalExpression(cond, left, right);
  }

  UnaryOperation* NewUnaryOperation(core::Token::Type op,
                                    Expression* expr) {
    return new (this) UnaryOperation(op, expr);
  }

  PostfixExpression* NewPostfixExpression(core::Token::Type op,
                                          Expression* expr) {
    return new (this) PostfixExpression(op, expr);
  }

  FunctionCall* NewFunctionCall(Expression* expr, Expressions* args) {
    return new (this) FunctionCall(expr, args);
  }

  ConstructorCall* NewConstructorCall(Expression* target, Expressions* args) {
    return new (this) ConstructorCall(target, args);
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


} }  // namespace iv::phonic
#endif  // _IV_PHONIC_FACTORY_H_
