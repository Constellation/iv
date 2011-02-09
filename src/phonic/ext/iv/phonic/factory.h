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
    : core::Space<2>() {
  }

  Scope* NewScope(FunctionLiteral::DeclType type) {
    return new (this) Scope(this, type == FunctionLiteral::GLOBAL);
  }

  template<typename Range>
  Identifier* NewIdentifier(core::Token::Type type,
                            const Range& range,
                            std::size_t begin,
                            std::size_t end) {
    Identifier* ident = new(this)Identifier(range, this);
    ident->set_type(type);
    ident->Location(begin, end);
    return ident;
  }

  NumberLiteral* NewNumberLiteral(const double& val,
                                  std::size_t begin,
                                  std::size_t end) {
    NumberLiteral* num = new (this) NumberLiteral(val);
    num->Location(begin, end);
    return num;
  }

  NumberLiteral* NewReducedNumberLiteral(const double& val) {
    UNREACHABLE();
    return NULL;
  }

  StringLiteral* NewStringLiteral(const std::vector<uc16>& buffer,
                                  std::size_t begin,
                                  std::size_t end) {
    StringLiteral* str = new (this) StringLiteral(buffer, this);
    str->Location(begin, end);
    return str;
  }

  Directivable* NewDirectivable(const std::vector<uc16>& buffer,
                                std::size_t begin,
                                std::size_t end) {
    Directivable* directive = new (this) Directivable(buffer, this);
    directive->Location(begin, end);
    return directive;
  }

  RegExpLiteral* NewRegExpLiteral(const std::vector<uc16>& content,
                                  const std::vector<uc16>& flags,
                                  std::size_t begin,
                                  std::size_t end) {
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
    RegExpLiteral* reg = new (this) RegExpLiteral(content, flags, this);
    reg->Location(begin, end);
    return reg;
  }

  FunctionLiteral* NewFunctionLiteral(FunctionLiteral::DeclType type,
                                      core::Maybe<Identifier> name,
                                      Identifiers* params,
                                      Statements* body,
                                      Scope* scope,
                                      bool strict,
                                      std::size_t begin_block_position,
                                      std::size_t end_block_position,
                                      std::size_t begin,
                                      std::size_t end) {
    FunctionLiteral* func = new (this)
        FunctionLiteral(type,
                        name,
                        params,
                        body,
                        scope,
                        strict,
                        begin_block_position,
                        end_block_position);
    func->Location(begin, end);
    return func;
  }

  ArrayLiteral* NewArrayLiteral(MaybeExpressions* items,
                                std::size_t begin, std::size_t end) {
    ArrayLiteral* ary = new (this) ArrayLiteral(items);
    ary->Location(begin, end);
    return ary;
  }


  ObjectLiteral*
      NewObjectLiteral(ObjectLiteral::Properties* properties,
                       std::size_t begin,
                       std::size_t end) {
    ObjectLiteral* obj = new (this) ObjectLiteral(properties);
    obj->Location(begin, end);
    return obj;
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

  NullLiteral* NewNullLiteral(std::size_t begin,
                              std::size_t end) {
    NullLiteral* null = new (this) NullLiteral();
    null->Location(begin, end);
    return null;
  }

  ThisLiteral* NewThisLiteral(std::size_t begin, std::size_t end) {
    ThisLiteral* th = new (this) ThisLiteral();
    th->Location(begin, end);
    return th;
  }

  TrueLiteral* NewTrueLiteral(std::size_t begin, std::size_t end) {
    TrueLiteral* tr = new (this) TrueLiteral();
    tr->Location(begin, end);
    return tr;
  }

  FalseLiteral* NewFalseLiteral(std::size_t begin, std::size_t end) {
    FalseLiteral* fal = new (this) FalseLiteral();
    fal->Location(begin, end);
    return fal;
  }

  EmptyStatement* NewEmptyStatement(std::size_t begin, std::size_t end) {
    EmptyStatement* empty = new (this) EmptyStatement();
    empty->Location(begin, end);
    return empty;
  }

  DebuggerStatement* NewDebuggerStatement(std::size_t begin, std::size_t end) {
    DebuggerStatement* debug = new (this) DebuggerStatement();
    debug->Location(begin, end);
    return debug;
  }

  FunctionStatement* NewFunctionStatement(FunctionLiteral* func) {
    FunctionStatement* stmt = new (this) FunctionStatement(func);
    stmt->Location(func->begin_position(), func->end_position());
    return stmt;
  }

  FunctionDeclaration* NewFunctionDeclaration(FunctionLiteral* func) {
    FunctionDeclaration* decl = new (this) FunctionDeclaration(func);
    decl->Location(func->begin_position(), func->end_position());
    return decl;
  }

  Block* NewBlock(Statements* body, std::size_t begin, std::size_t end) {
    Block* block = new (this) Block(body);
    block->Location(begin, end);
    return block;
  }

  VariableStatement* NewVariableStatement(core::Token::Type token,
                                          Declarations* decls,
                                          std::size_t begin,
                                          std::size_t end) {
    assert(!decls->empty());
    VariableStatement* var = new (this) VariableStatement(token, decls);
    var->Location(begin, end);
    return var;
  }

  Declaration* NewDeclaration(Identifier* name, core::Maybe<Expression> expr) {
    assert(name);
    Declaration* decl = new (this) Declaration(name, expr);
    const std::size_t end = (expr) ? (*expr).end_position() : name->end_position();
    decl->Location(name->begin_position(), end);
    return decl;
  }

  IfStatement* NewIfStatement(Expression* cond,
                              Statement* then_statement,
                              core::Maybe<Statement> else_statement,
                              std::size_t begin) {
    IfStatement* stmt = new (this) IfStatement(cond,
                                               then_statement, else_statement);
    assert(then_statement);
    const std::size_t end = (else_statement) ?
        (*else_statement).end_position() : then_statement->end_position();
    stmt->Location(begin, end);
    return stmt;
  }

  DoWhileStatement* NewDoWhileStatement(Statement* body,
                                        Expression* cond,
                                        std::size_t begin,
                                        std::size_t end) {
    DoWhileStatement* stmt = new (this) DoWhileStatement(body, cond);
    stmt->Location(begin, end);
    return stmt;
  }

  WhileStatement* NewWhileStatement(Statement* body,
                                    Expression* cond,
                                    std::size_t begin) {
    assert(body && cond);
    WhileStatement* stmt = new (this) WhileStatement(body, cond);
    stmt->Location(begin, body->end_position());
    return stmt;
  }

  ForInStatement* NewForInStatement(Statement* body,
                                    Statement* each,
                                    Expression* enumerable,
                                    std::size_t begin) {
    assert(body);
    ForInStatement* stmt = new (this) ForInStatement(body, each, enumerable);
    stmt->Location(begin, body->end_position());
    return stmt;
  }

  ForStatement* NewForStatement(Statement* body,
                                core::Maybe<Statement> init,
                                core::Maybe<Expression> cond,
                                core::Maybe<Statement> next,
                                std::size_t begin) {
    assert(body);
    ForStatement* stmt = new (this) ForStatement(body, init, cond, next);
    stmt->Location(begin, body->end_position());
    return stmt;
  }

  ExpressionStatement* NewExpressionStatement(Expression* expr, std::size_t end) {
    ExpressionStatement* stmt = new (this) ExpressionStatement(expr);
    stmt->Location(expr->begin_position(), end);
    return stmt;
  }

  ContinueStatement* NewContinueStatement(core::Maybe<Identifier> label,
                                          IterationStatement** target,
                                          std::size_t begin,
                                          std::size_t end) {
    ContinueStatement* stmt = new (this) ContinueStatement(label, target);
    stmt->Location(begin, end);
    return stmt;
  }

  BreakStatement* NewBreakStatement(core::Maybe<Identifier> label,
                                    BreakableStatement** target,
                                    std::size_t begin,
                                    std::size_t end) {
    BreakStatement* stmt = new (this) BreakStatement(label, target);
    stmt->Location(begin, end);
    return stmt;
  }

  ReturnStatement* NewReturnStatement(core::Maybe<Expression> expr,
                                      std::size_t begin,
                                      std::size_t end) {
    ReturnStatement* stmt = new (this) ReturnStatement(expr);
    stmt->Location(begin, end);
    return stmt;
  }

  WithStatement* NewWithStatement(Expression* expr,
                                  Statement* body, std::size_t begin) {
    assert(body);
    WithStatement* stmt = new (this) WithStatement(expr, body);
    stmt->Location(begin, body->end_position());
    return stmt;
  }

  SwitchStatement* NewSwitchStatement(Expression* expr, CaseClauses* clauses,
                                      std::size_t begin, std::size_t end) {
    SwitchStatement* stmt = new (this) SwitchStatement(expr, clauses);
    stmt->Location(begin, end);
    return stmt;
  }

  CaseClause* NewCaseClause(bool is_default,
                            core::Maybe<Expression> expr, Statements* body,
                            std::size_t begin,
                            std::size_t end) {
    CaseClause* clause = new (this) CaseClause(is_default, expr, body);
    clause->Location(begin, end);
    return clause;
  }


  ThrowStatement*  NewThrowStatement(Expression* expr,
                                     std::size_t begin,
                                     std::size_t end) {
    assert(expr);
    ThrowStatement* stmt = new (this) ThrowStatement(expr);
    stmt->Location(begin, end);
    return stmt;
  }

  TryStatement* NewTryStatement(Block* try_block,
                                core::Maybe<Identifier> catch_name,
                                core::Maybe<Block> catch_block,
                                core::Maybe<Block> finally_block,
                                std::size_t begin) {
    TryStatement* stmt = new (this) TryStatement(try_block,
                                                 catch_name,
                                                 catch_block,
                                                 finally_block);
    assert(catch_block || finally_block);
    const std::size_t end = (finally_block) ?
        (*finally_block).end_position() : (*catch_block).end_position();
    stmt->Location(begin, end);
    return stmt;
  }

  LabelledStatement* NewLabelledStatement(Expression* expr, Statement* stmt) {
    LabelledStatement* label = new (this) LabelledStatement(expr, stmt);
    label->Location(expr->begin_position(), stmt->end_position());
    return label;
  }

  BinaryOperation* NewBinaryOperation(core::Token::Type op,
                                      Expression* result,
                                      Expression* right) {
    BinaryOperation* expr = new (this) BinaryOperation(op, result, right);
    expr->Location(result->begin_position(), right->end_position());
    return expr;
  }

  Assignment* NewAssignment(core::Token::Type op,
                            Expression* left,
                            Expression* right) {
    Assignment* expr = new (this) Assignment(op, left, right);
    expr->Location(left->begin_position(), right->end_position());
    return expr;
  }

  ConditionalExpression* NewConditionalExpression(Expression* cond,
                                                  Expression* left,
                                                  Expression* right) {
    ConditionalExpression* expr = new (this) ConditionalExpression(cond, left, right);
    expr->Location(cond->begin_position(), right->end_position());
    return expr;
  }

  UnaryOperation* NewUnaryOperation(core::Token::Type op,
                                    Expression* expr, std::size_t begin) {
    assert(expr);
    UnaryOperation* unary = new (this) UnaryOperation(op, expr);
    unary->Location(begin, expr->end_position());
    return unary;
  }

  PostfixExpression* NewPostfixExpression(core::Token::Type op,
                                          Expression* expr, std::size_t end) {
    assert(expr);
    PostfixExpression* post = new (this) PostfixExpression(op, expr);
    post->Location(expr->begin_position(), end);
    return post;
  }

  FunctionCall* NewFunctionCall(Expression* expr,
                                Expressions* args, std::size_t end) {
    FunctionCall* call = new (this) FunctionCall(expr, args);
    call->Location(expr->begin_position(), end);
    return call;
  }

  ConstructorCall* NewConstructorCall(Expression* target,
                                      Expressions* args, std::size_t end) {
    ConstructorCall* call = new (this) ConstructorCall(target, args);
    call->Location(target->begin_position(), end);
    return call;
  }

  IndexAccess* NewIndexAccess(Expression* expr, Expression* index) {
    IndexAccess* access = new (this) IndexAccess(expr, index);
    access->Location(expr->begin_position(), index->end_position());
    return access;
  }

  IdentifierAccess* NewIdentifierAccess(Expression* expr, Identifier* ident) {
    IdentifierAccess* access = new (this) IdentifierAccess(expr, ident);
    access->Location(expr->begin_position(), ident->end_position());
    return access;
  }
};


} }  // namespace iv::phonic
#endif  // _IV_PHONIC_FACTORY_H_
