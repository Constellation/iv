#ifndef IV_AST_FACTORY_H_
#define IV_AST_FACTORY_H_
#include <iv/detail/type_traits.h>
#include <iv/functor.h>
#include <iv/location.h>
#include <iv/ast.h>
#include <iv/alloc.h>
#include <iv/static_assert.h>
#include <iv/ustringpiece.h>

namespace iv {
namespace core {
namespace ast {

template<typename Factory>
class BasicAstFactory {
 public:
  typedef BasicAstFactory<Factory> this_type;

#define V(AST) typedef typename ast::AST<Factory> AST;
  IV_AST_NODE_LIST(V)
#undef V
#define V(XS) typedef typename ast::AstNode<Factory>::XS XS;
  IV_AST_LIST_LIST(V)
#undef V
#define V(S) typedef typename SpaceUString<Factory>::type S;
  IV_AST_STRING(V)
#undef V

  BasicAstFactory() {
    IV_STATIC_ASSERT((std::is_convertible<Factory, this_type>::value) ||
                     (std::is_base_of<this_type, Factory>::value));
  }

  Identifier* NewIdentifier(Token::Type token,
                            Symbol symbol,
                            std::size_t begin,
                            std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this)) Identifier(symbol),
        begin, end);
  }

  Assigned* NewAssigned(const SymbolHolder& holder, bool immutable) {
    return Location(
        new(static_cast<Factory*>(this)) Assigned(holder, immutable),
        holder.begin_position(), holder.end_position());
  }

  NumberLiteral* NewReducedNumberLiteral(const double& val) {
    return Location(
        new(static_cast<Factory*>(this)) ReducedNumberLiteral(val), 0, 0);
  }

  NumberLiteral* NewNumberLiteral(const double& val,
                                  std::size_t begin, std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this)) NumberLiteral(val),
        begin, end);
  }


  StringLiteral* NewStringLiteral(const std::vector<uint16_t>& buffer,
                                  std::size_t begin, std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this)) StringLiteral(NewString(buffer), true),
        begin, end);
  }

  StringLiteral* NewReducedStringLiteral(StringLiteral* lhs,
                                         StringLiteral* rhs) {
    return Location(
        new(static_cast<Factory*>(this))
          ReducedStringLiteral(NewString(lhs, rhs), false), 0, 0);
  }

  RegExpLiteral* NewRegExpLiteral(const std::vector<uint16_t>& content,
                                  const std::vector<uint16_t>& flags,
                                  std::size_t begin,
                                  std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this))
        RegExpLiteral(NewString(content), NewString(flags)),
        begin, end);
  }

  FunctionLiteral* NewFunctionLiteral(typename FunctionLiteral::DeclType type,
                                      Maybe<Assigned> name,
                                      Assigneds* params,
                                      Statements* body,
                                      Scope* scope,
                                      bool strict,
                                      std::size_t begin_block_position,
                                      std::size_t end_block_position,
                                      std::size_t begin,
                                      std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this))
        FunctionLiteral(type,
                        name,
                        params,
                        body,
                        scope,
                        strict,
                        begin_block_position,
                        end_block_position),
        begin, end);
  }

  ArrayLiteral* NewArrayLiteral(MaybeExpressions* items,
                                std::size_t begin, std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this)) ArrayLiteral(items),
        begin, end);
  }

  ObjectLiteral* NewObjectLiteral(
      typename ObjectLiteral::Properties* properties,
      std::size_t begin, std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this)) ObjectLiteral(properties),
        begin, end);
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

  template<typename Range>
  const SpaceUString* NewString(const Range& range) {
    return new (static_cast<Factory*>(this)->New(sizeof(SpaceUString)))
        SpaceUString(
            range.begin(),
            range.end(),
            typename SpaceUString::allocator_type(static_cast<Factory*>(this)));
  }

  const SpaceUString* NewString(StringLiteral* lhs, StringLiteral* rhs) {
    SpaceUString* str =
        new(static_cast<Factory*>(this)->New(sizeof(SpaceUString)))
          SpaceUString(
            lhs->value().size() + rhs->value().size(),
            'C',
            typename SpaceUString::allocator_type(static_cast<Factory*>(this)));
    std::copy(
        rhs->value().begin(),
        rhs->value().end(),
        std::copy(lhs->value().begin(), lhs->value().end(), str->begin()));
    return str;
  }

  Scope* NewScope(typename FunctionLiteral::DeclType type, Assigneds* params) {
    return new (static_cast<Factory*>(this))
        Scope(static_cast<Factory*>(this),
              type == FunctionLiteral::GLOBAL, params);
  }

  NullLiteral* NewNullLiteral(std::size_t begin, std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this)) NullLiteral(),
        begin, end);
  }

  ThisLiteral* NewThisLiteral(std::size_t begin, std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this)) ThisLiteral(),
        begin, end);
  }

  TrueLiteral* NewTrueLiteral(std::size_t begin, std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this)) TrueLiteral(),
        begin, end);
  }

  FalseLiteral* NewFalseLiteral(std::size_t begin, std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this)) FalseLiteral(),
        begin, end);
  }

  EmptyStatement* NewEmptyStatement(std::size_t begin, std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this)) EmptyStatement(),
        begin, end);
  }

  DebuggerStatement* NewDebuggerStatement(std::size_t begin, std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this)) DebuggerStatement(),
        begin, end);
  }

  // if you want begin / end position,
  // set position to FunctionLiteral in NewFunctionLiteral and use it
  FunctionStatement* NewFunctionStatement(FunctionLiteral* func) {
    return Location(
        new(static_cast<Factory*>(this)) FunctionStatement(func),
        func->begin_position(), func->end_position());
  }

  // if you want begin / end position,
  // set position to FunctionLiteral in NewFunctionLiteral and use it
  FunctionDeclaration* NewFunctionDeclaration(FunctionLiteral* func) {
    return Location(
        new(static_cast<Factory*>(this)) FunctionDeclaration(func),
        func->begin_position(), func->end_position());
  }

  Block* NewBlock(Statements* body, std::size_t begin, std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this)) Block(body),
        begin, end);
  }

  VariableStatement* NewVariableStatement(Token::Type token,
                                          Declarations* decls,
                                          std::size_t begin,
                                          std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this)) VariableStatement(token, decls),
        begin, end);
  }

  Declaration* NewDeclaration(Assigned* name,
                              Maybe<Expression> expr) {
    return Location(
        new(static_cast<Factory*>(this)) Declaration(name, expr),
        name->begin_position(),
        (expr) ? (*expr).end_position() : name->end_position());
  }

  IfStatement* NewIfStatement(Expression* cond,
                              Statement* then_statement,
                              Maybe<Statement> else_statement,
                              std::size_t begin) {
    return Location(
        new(static_cast<Factory*>(this)) IfStatement(cond,
                                                     then_statement,
                                                     else_statement),
        begin,
        (else_statement) ?
        (*else_statement).end_position() : then_statement->end_position());
  }

  DoWhileStatement* NewDoWhileStatement(Statement* body,
                                        Expression* cond,
                                        std::size_t begin,
                                        std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this)) DoWhileStatement(body, cond),
        begin, end);
  }

  WhileStatement* NewWhileStatement(Statement* body,
                                    Expression* cond,
                                    std::size_t begin) {
    return Location(
        new(static_cast<Factory*>(this)) WhileStatement(body, cond),
        begin, body->end_position());
  }

  ForInStatement* NewForInStatement(Statement* body,
                                    Statement* each,
                                    Expression* enumerable,
                                    std::size_t begin) {
    return Location(
        new(static_cast<Factory*>(this)) ForInStatement(body,
                                                        each, enumerable),
        begin, body->end_position());
  }

  ForStatement* NewForStatement(Statement* body,
                                Maybe<Statement> init,
                                Maybe<Expression> cond,
                                Maybe<Expression> next,
                                std::size_t begin) {
    return Location(
        new(static_cast<Factory*>(this)) ForStatement(body, init,
                                                      cond, next),
        begin, body->end_position());
  }

  ExpressionStatement* NewExpressionStatement(Expression* expr,
                                              std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this)) ExpressionStatement(expr),
        expr->begin_position(), end);
  }

  ContinueStatement* NewContinueStatement(const SymbolHolder& label,
                                          IterationStatement** target,
                                          std::size_t begin,
                                          std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this)) ContinueStatement(label, target),
        begin, end);
  }

  BreakStatement* NewBreakStatement(const SymbolHolder& label,
                                    BreakableStatement** target,
                                    std::size_t begin,
                                    std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this)) BreakStatement(label, target),
        begin, end);
  }

  ReturnStatement* NewReturnStatement(Maybe<Expression> expr,
                                      std::size_t begin,
                                      std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this)) ReturnStatement(expr),
        begin, end);
  }

  WithStatement* NewWithStatement(Expression* expr,
                                  Statement* stmt, std::size_t begin) {
    return Location(
        new(static_cast<Factory*>(this)) WithStatement(expr, stmt),
        begin, stmt->end_position());
  }

  SwitchStatement* NewSwitchStatement(Expression* expr, CaseClauses* clauses,
                                      std::size_t begin, std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this)) SwitchStatement(expr, clauses),
        begin, end);
  }

  CaseClause* NewCaseClause(bool is_default,
                            Maybe<Expression> expr, Statements* body,
                            std::size_t begin,
                            std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this)) CaseClause(is_default, expr, body),
        begin, end);
  }

  ThrowStatement*  NewThrowStatement(Expression* expr,
                                     std::size_t begin, std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this)) ThrowStatement(expr),
        begin, end);
  }

  TryStatement* NewTryStatement(Block* try_block,
                                Maybe<Assigned> catch_name,
                                Maybe<Block> catch_block,
                                Maybe<Block> finally_block,
                                std::size_t begin) {
    return Location(
        new(static_cast<Factory*>(this)) TryStatement(try_block,
                                                      catch_name,
                                                      catch_block,
                                                      finally_block),
        begin,
        (finally_block) ?
        (*finally_block).end_position() : (*catch_block).end_position());
  }

  LabelledStatement* NewLabelledStatement(const SymbolHolder& label,
                                          Statement* stmt) {
    return Location(
        new(static_cast<Factory*>(this)) LabelledStatement(label, stmt),
        label.begin_position(), stmt->end_position());
  }

  BinaryOperation* NewBinaryOperation(Token::Type op,
                                      Expression* result, Expression* right) {
    return Location(
        new(static_cast<Factory*>(this)) BinaryOperation(op, result, right),
        result->begin_position(), right->end_position());
  }

  Assignment* NewAssignment(Token::Type op,
                            Expression* left,
                            Expression* right) {
    return Location(
        new(static_cast<Factory*>(this)) Assignment(op, left, right),
        left->begin_position(), right->end_position());
  }

  ConditionalExpression* NewConditionalExpression(Expression* cond,
                                                  Expression* left,
                                                  Expression* right) {
    return Location(
        new(static_cast<Factory*>(this)) ConditionalExpression(cond,
                                                               left, right),
        cond->begin_position(), right->end_position());
  }

  UnaryOperation* NewUnaryOperation(Token::Type op,
                                    Expression* expr, std::size_t begin) {
    return Location(
        new(static_cast<Factory*>(this)) UnaryOperation(op, expr),
        begin, expr->end_position());
  }

  PostfixExpression* NewPostfixExpression(Token::Type op,
                                          Expression* expr, std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this)) PostfixExpression(op, expr),
        expr->begin_position(), end);
  }

  FunctionCall* NewFunctionCall(Expression* expr,
                                Expressions* args, std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this)) FunctionCall(expr, args),
        expr->begin_position(), end);
  }

  // !!! CAUTION !!!
  // if not right paren (like new Array), end is 0
  ConstructorCall* NewConstructorCall(Expression* target,
                                      Expressions* args, std::size_t end) {
    return Location(
        new(static_cast<Factory*>(this)) ConstructorCall(target, args),
        target->begin_position(), (end) ? end : target->end_position());
  }

  IndexAccess* NewIndexAccess(Expression* expr, Expression* index) {
    return Location(
        new(static_cast<Factory*>(this)) IndexAccess(expr, index),
        expr->begin_position(), index->end_position());
  }

  IdentifierAccess* NewIdentifierAccess(Expression* expr,
                                        const SymbolHolder& ident) {
    return Location(
        new(static_cast<Factory*>(this)) IdentifierAccess(expr, ident),
        expr->begin_position(), ident.end_position());
  }

 private:
  template<typename T>
  T* Location(T* node, std::size_t begin, std::size_t end) {
    node->set_position(begin, end);
    return node;
  }
};

template<typename Derived>
class AstFactory : public core::Space, public BasicAstFactory<Derived> { };

} } }  // namespace iv::core::ast
#endif  // IV_AST_FACTORY_H_
