#ifndef _IV_PHONIC_FACTORY_H_
#define _IV_PHONIC_FACTORY_H_
#include <tr1/array>
#include <ruby.h>
#include <ruby/oniguruma.h>
#include <iv/alloc.h>
#include <iv/ustringpiece.h>
#include "encoding.h"
#include "ast.h"
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

  Identifier* NewIdentifier(const core::Location& location,
                            const core::UStringPiece& buffer) {
    Identifier* ident = new(this)Identifier(buffer, this);
    return ident;
  }

  Identifier* NewIdentifier(const core::Location& location,
                            const std::vector<uc16>& buffer) {
    Identifier* ident = new(this)Identifier(buffer, this);
    return ident;
  }

  Identifier* NewIdentifier(const core::Location& location,
                            const std::vector<char>& buffer) {
    Identifier* ident = new(this)Identifier(buffer, this);
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

  ArrayLiteral* NewArrayLiteral() {
    return new (this) ArrayLiteral(this);
  }

  ObjectLiteral* NewObjectLiteral() {
    return new (this) ObjectLiteral(this);
  }

  Identifiers* NewLabels() {
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

  VariableStatement* NewVariableStatement(core::Token::Type token) {
    return new (this) VariableStatement(token, this);
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

  DoWhileStatement* NewDoWhileStatement() {
    return new (this) DoWhileStatement();
  }

  WhileStatement* NewWhileStatement() {
    return new (this) WhileStatement();
  }

  ForInStatement* NewForInStatement() {
    return new (this) ForInStatement();
  }

  ExpressionStatement* NewExpressionStatement(Expression* expr) {
    return new (this) ExpressionStatement(expr);
  }

  ForStatement* NewForStatement() {
    return new (this) ForStatement();
  }

  ContinueStatement* NewContinueStatement(Identifier* label,
                                          IterationStatement* target) {
    return new (this) ContinueStatement(label, target);
  }

  BreakStatement* NewBreakStatement(Identifier* label,
                                    BreakableStatement* target) {
    return new (this) BreakStatement(label, target);
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

  CaseClause* NewCaseClause(bool is_default, Expression* expr) {
    return new (this) CaseClause(is_default, expr, this);
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


} }  // namespace iv::phonic
#endif  // _IV_PHONIC_FACTORY_H_
