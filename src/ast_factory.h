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
#define V(XS) typedef typename ast::AstNode<Factory>::XS XS;
  AST_LIST_LIST(V)
#undef V
#define V(S) typedef typename SpaceUString<Factory>::type S;
  AST_STRING(V)
#undef V

  BasicAstFactory()
    : empty_statement_instance_(
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
  Identifier* NewIdentifier(Token::Type token,
                            const Range& range,
                            std::size_t begin,
                            std::size_t end) {
    return new (static_cast<Factory*>(this))
        Identifier(range, static_cast<Factory*>(this));
  }

  NumberLiteral* NewReducedNumberLiteral(const double& val) {
    return new (static_cast<Factory*>(this)) NumberLiteral(val);
  }

  NumberLiteral* NewNumberLiteral(const double& val,
                                  std::size_t begin, std::size_t end) {
    return new (static_cast<Factory*>(this)) NumberLiteral(val);
  }


  StringLiteral* NewStringLiteral(const std::vector<uc16>& buffer,
                                  std::size_t begin, std::size_t end) {
    return new (static_cast<Factory*>(this))
        StringLiteral(buffer, static_cast<Factory*>(this));
  }

  Directivable* NewDirectivable(const std::vector<uc16>& buffer,
                                std::size_t begin, std::size_t end) {
    return new (static_cast<Factory*>(this))
        Directivable(buffer, static_cast<Factory*>(this));
  }

  RegExpLiteral* NewRegExpLiteral(const std::vector<uc16>& content,
                                  const std::vector<uc16>& flags,
                                  std::size_t begin,
                                  std::size_t end) {
    return new (static_cast<Factory*>(this))
        RegExpLiteral(content, flags, static_cast<Factory*>(this));
  }

  FunctionLiteral* NewFunctionLiteral(typename FunctionLiteral::DeclType type,
                                      Maybe<Identifier> name,
                                      Identifiers* params,
                                      Statements* body,
                                      Scope* scope,
                                      bool strict,
                                      std::size_t begin_block_position,
                                      std::size_t end_block_position,
                                      std::size_t begin,
                                      std::size_t end) {
    return new (static_cast<Factory*>(this))
        FunctionLiteral(type,
                        name,
                        params,
                        body,
                        scope,
                        strict,
                        begin_block_position,
                        end_block_position);
  }

  ArrayLiteral* NewArrayLiteral(MaybeExpressions* items,
                                std::size_t begin, std::size_t end) {
    return new (static_cast<Factory*>(this)) ArrayLiteral(items);
  }

  ObjectLiteral*
      NewObjectLiteral(typename ObjectLiteral::Properties* properties,
                       std::size_t begin,
                       std::size_t end) {
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

  Scope* NewScope(typename FunctionLiteral::DeclType type) {
    return new (static_cast<Factory*>(this))
        Scope(static_cast<Factory*>(this), type == FunctionLiteral::GLOBAL);
  }

  NullLiteral* NewNullLiteral(std::size_t begin,
                              std::size_t end) {
    return null_instance_;
  }

  ThisLiteral* NewThisLiteral(std::size_t begin, std::size_t end) {
    return this_instance_;
  }

  TrueLiteral* NewTrueLiteral(std::size_t begin, std::size_t end) {
    return true_instance_;
  }

  FalseLiteral* NewFalseLiteral(std::size_t begin, std::size_t end) {
    return false_instance_;
  }

  EmptyStatement* NewEmptyStatement(std::size_t begin, std::size_t end) {
    return empty_statement_instance_;
  }

  DebuggerStatement* NewDebuggerStatement(std::size_t begin, std::size_t end) {
    return debugger_statement_instance_;
  }

  // if you want begin / end position,
  // set position to FunctionLiteral in NewFunctionLiteral and use it
  FunctionStatement* NewFunctionStatement(FunctionLiteral* func) {
    return new (static_cast<Factory*>(this)) FunctionStatement(func);
  }

  // if you want begin / end position,
  // set position to FunctionLiteral in NewFunctionLiteral and use it
  FunctionDeclaration* NewFunctionDeclaration(FunctionLiteral* func) {
    return new (static_cast<Factory*>(this)) FunctionDeclaration(func);
  }

  Block* NewBlock(Statements* body, std::size_t begin, std::size_t end) {
    return new (static_cast<Factory*>(this)) Block(body);
  }

  VariableStatement* NewVariableStatement(Token::Type token,
                                          Declarations* decls,
                                          std::size_t begin,
                                          std::size_t end) {
    return new (static_cast<Factory*>(this))
        VariableStatement(token, decls);
  }

  // if you want begin / end position,
  // set position to Identifier / Expression and use it
  Declaration* NewDeclaration(Identifier* name, Maybe<Expression> expr) {
    return new (static_cast<Factory*>(this)) Declaration(name, expr);
  }

  // if you want end position,
  // set position to Statement and use it
  IfStatement* NewIfStatement(Expression* cond,
                              Statement* then_statement,
                              Maybe<Statement> else_statement,
                              std::size_t begin) {
    return new (static_cast<Factory*>(this)) IfStatement(cond,
                                                         then_statement,
                                                         else_statement);
  }

  DoWhileStatement* NewDoWhileStatement(Statement* body,
                                        Expression* cond,
                                        std::size_t begin,
                                        std::size_t end) {
    return new (static_cast<Factory*>(this)) DoWhileStatement(body, cond);
  }

  // if you want end position,
  // set position to Statement and use it
  WhileStatement* NewWhileStatement(Statement* body,
                                    Expression* cond,
                                    std::size_t begin) {
    return new (static_cast<Factory*>(this)) WhileStatement(body, cond);
  }

  // if you want end position,
  // set position to Statement and use it
  ForInStatement* NewForInStatement(Statement* body,
                                    Statement* each,
                                    Expression* enumerable,
                                    std::size_t begin) {
    return new (static_cast<Factory*>(this)) ForInStatement(body,
                                                            each, enumerable);
  }

  // if you want end position,
  // set position to Statement and use it
  ForStatement* NewForStatement(Statement* body,
                                Maybe<Statement> init,
                                Maybe<Expression> cond,
                                Maybe<Statement> next,
                                std::size_t begin) {
    return new (static_cast<Factory*>(this)) ForStatement(body, init,
                                                          cond, next);
  }

  ExpressionStatement* NewExpressionStatement(Expression* expr, std::size_t end) {
    return new (static_cast<Factory*>(this)) ExpressionStatement(expr);
  }

  ContinueStatement* NewContinueStatement(Maybe<Identifier> label,
                                          IterationStatement** target,
                                          std::size_t begin,
                                          std::size_t end) {
    return new (static_cast<Factory*>(this)) ContinueStatement(label, target);
  }

  BreakStatement* NewBreakStatement(Maybe<Identifier> label,
                                    BreakableStatement** target,
                                    std::size_t begin,
                                    std::size_t end) {
    return new (static_cast<Factory*>(this)) BreakStatement(label, target);
  }

  ReturnStatement* NewReturnStatement(Maybe<Expression> expr,
                                      std::size_t begin,
                                      std::size_t end) {
    return new (static_cast<Factory*>(this)) ReturnStatement(expr);
  }

  // if you want end position,
  // set position to Expression and use it
  WithStatement* NewWithStatement(Expression* expr,
                                  Statement* stmt, std::size_t begin) {
    return new (static_cast<Factory*>(this)) WithStatement(expr, stmt);
  }

  SwitchStatement* NewSwitchStatement(Expression* expr, CaseClauses* clauses,
                                      std::size_t begin, std::size_t end) {
    return new (static_cast<Factory*>(this)) SwitchStatement(expr, clauses);
  }

  // !!! if body is empty, end_position is end.
  CaseClause* NewCaseClause(bool is_default,
                            Maybe<Expression> expr, Statements* body,
                            std::size_t begin,
                            std::size_t end) {
    return new (static_cast<Factory*>(this)) CaseClause(is_default, expr, body);
  }

  ThrowStatement*  NewThrowStatement(Expression* expr,
                                     std::size_t begin, std::size_t end) {
    return new (static_cast<Factory*>(this)) ThrowStatement(expr);
  }

  // if you want end position,
  // set position to Block and use it
  TryStatement* NewTryStatement(Block* try_block,
                                Maybe<Identifier> catch_name,
                                Maybe<Block> catch_block,
                                Maybe<Block> finally_block,
                                std::size_t begin) {
    return new (static_cast<Factory*>(this)) TryStatement(try_block,
                                                          catch_name,
                                                          catch_block,
                                                          finally_block);
  }

  // if you want begin / end position,
  // set position to Expression and use it
  LabelledStatement* NewLabelledStatement(Expression* expr, Statement* stmt) {
    return new (static_cast<Factory*>(this)) LabelledStatement(expr, stmt);
  }

  // if you want begin / end position,
  // set position to Expression and use it
  BinaryOperation* NewBinaryOperation(Token::Type op,
                                      Expression* result, Expression* right) {
    return new (static_cast<Factory*>(this)) BinaryOperation(op, result, right);
  }

  // if you want begin / end position,
  // set position to Expression and use it
  Assignment* NewAssignment(Token::Type op,
                            Expression* left,
                            Expression* right) {
    return new (static_cast<Factory*>(this)) Assignment(op, left, right);
  }

  // if you want begin / end position,
  // set position to Expression and use it
  ConditionalExpression* NewConditionalExpression(Expression* cond,
                                                  Expression* left,
                                                  Expression* right) {
    return new (static_cast<Factory*>(this))
        ConditionalExpression(cond, left, right);
  }

  // if you want end position,
  // set position to Expression and use it
  UnaryOperation* NewUnaryOperation(Token::Type op,
                                    Expression* expr, std::size_t begin) {
    return new (static_cast<Factory*>(this)) UnaryOperation(op, expr);
  }

  // if you want begin position,
  // set position to Expression and use it
  PostfixExpression* NewPostfixExpression(Token::Type op,
                                          Expression* expr, std::size_t end) {
    return new (static_cast<Factory*>(this)) PostfixExpression(op, expr);
  }

  // if you want begin position,
  // set position to Expression and use it
  FunctionCall* NewFunctionCall(Expression* expr,
                                Expressions* args, std::size_t end) {
    return new (static_cast<Factory*>(this)) FunctionCall(expr, args);
  }

  // if you want begin position,
  // set position to Expression and use it
  // !!! CAUTION !!!
  // if not right paren (like new Array), end is 0
  ConstructorCall* NewConstructorCall(Expression* target,
                                      Expressions* args, std::size_t end) {
    return new (static_cast<Factory*>(this)) ConstructorCall(target, args);
  }

  // if you want begin / end position,
  // set position to Expression / Identifier and use it
  IndexAccess* NewIndexAccess(Expression* expr,
                              Expression* index) {
    return new (static_cast<Factory*>(this)) IndexAccess(expr, index);
  }

  // if you want begin / end position,
  // set position to Expression / Identifier and use it
  IdentifierAccess* NewIdentifierAccess(Expression* expr,
                                        Identifier* ident) {
    return new (static_cast<Factory*>(this)) IdentifierAccess(expr, ident);
  }

 private:
  EmptyStatement* empty_statement_instance_;
  DebuggerStatement* debugger_statement_instance_;
  ThisLiteral* this_instance_;
  NullLiteral* null_instance_;
  TrueLiteral* true_instance_;
  FalseLiteral* false_instance_;
};

} } }  // namespace iv::core::ast
#endif  // _IV_AST_FACTORY_H_
