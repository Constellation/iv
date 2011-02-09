#ifndef _IV_AST_H_
#define _IV_AST_H_
#include <vector>
#include <tr1/unordered_map>
#include <tr1/tuple>
#include <tr1/type_traits>
#include "uchar.h"
#include "noncopyable.h"
#include "utils.h"
#include "space.h"
#include "maybe.h"
#include "functor.h"
#include "token.h"
#include "ast_fwd.h"
#include "ast_visitor.h"
#include "location.h"
#include "static_assert.h"
#include "ustringpiece.h"

namespace iv {
namespace core {
namespace ast {
#define ACCEPT_VISITOR \
  inline void Accept(\
      typename AstVisitor<Factory>::type* visitor) {\
    visitor->Visit(this);\
  }\
  inline void Accept(\
      typename AstVisitor<Factory>::const_type * visitor) const {\
    visitor->Visit(this);\
  }

#define DECLARE_NODE_TYPE(type) \
  inline const type<Factory>* As##type() const { return this; }\
  inline type<Factory>* As##type() { return this; }\

#define DECLARE_DERIVED_NODE_TYPE(type) \
  DECLARE_NODE_TYPE(type)\
  ACCEPT_VISITOR\
  inline NodeType Type() const { return k##type; }

#define DECLARE_NODE_TYPE_BASE(type) \
  inline virtual const type<Factory>* As##type() const { return NULL; }\
  inline virtual type<Factory>* As##type() { return NULL; }

enum NodeType {
#define V(type)\
  k##type,
AST_NODE_LIST(V)
#undef V
  kNodeCount
};

template<typename Factory, NodeType type>
class Inherit {
};

#define INHERIT(type)\
template<typename Factory>\
class type##Base\
  : public Inherit<Factory, k##type> {\
}

template<typename Factory>
class Scope : public SpaceObject,
              private Noncopyable<Scope<Factory> >::type {
 public:
  typedef std::pair<Identifier<Factory>*, bool> Variable;
  typedef typename SpaceVector<Factory, Variable>::type Variables;
  typedef typename SpaceVector<
            Factory,
            FunctionLiteral<Factory>*>::type FunctionLiterals;
  typedef Scope<Factory> this_type;

  explicit Scope(Factory* factory, bool is_global)
    : up_(NULL),
      vars_(typename Variables::allocator_type(factory)),
      funcs_(typename FunctionLiterals::allocator_type(factory)),
      is_global_(is_global) {
  }
  void AddUnresolved(Identifier<Factory>* name, bool is_const) {
    vars_.push_back(std::make_pair(name, is_const));
  }
  void AddFunctionDeclaration(FunctionLiteral<Factory>* func) {
    funcs_.push_back(func);
  }
  void SetUpperScope(this_type* scope) {
    up_ = scope;
  }
  inline const FunctionLiterals& function_declarations() const {
    return funcs_;
  }
  inline const Variables& variables() const {
    return vars_;
  }
  inline bool IsGlobal() const {
    return is_global_;
  }
  this_type* GetUpperScope() {
    return up_;
  }
 protected:
  this_type* up_;
  Variables vars_;
  FunctionLiterals funcs_;
  bool is_global_;
};

template<typename Factory>
class Inherit<Factory, kAstNode>
  : public SpaceObject,
    private Noncopyable<Inherit<Factory, kAstNode> >::type {
};
INHERIT(AstNode);

template<typename Factory>
class AstNode : public AstNodeBase<Factory> {
 public:
  virtual ~AstNode() = 0;

  typedef typename SpaceVector<Factory, Statement<Factory>*>::type Statements;
  typedef typename SpaceVector<Factory, Identifier<Factory>*>::type Identifiers;
  typedef typename SpaceVector<Factory, Expression<Factory>*>::type Expressions;
  typedef typename SpaceVector<Factory,
                               Maybe<Expression<Factory> > >::type MaybeExpressions;
  typedef typename SpaceVector<Factory,
                               Declaration<Factory>*>::type Declarations;
  typedef typename SpaceVector<Factory, CaseClause<Factory>*>::type CaseClauses;

  STATEMENT_NODE_LIST(DECLARE_NODE_TYPE_BASE)
  EXPRESSION_NODE_LIST(DECLARE_NODE_TYPE_BASE)

  virtual void Accept(
      typename AstVisitor<Factory>::type* visitor) = 0;
  virtual void Accept(
      typename AstVisitor<Factory>::const_type* visitor) const = 0;
  virtual NodeType Type() const = 0;
};

template<typename Factory>
inline AstNode<Factory>::~AstNode() { }

#undef DECLARE_NODE_TYPE_BASE
//  Statement
//    : Block
//    | FunctionStatement
//    | VariableStatement
//    | EmptyStatement
//    | ExpressionStatement
//    | IfStatement
//    | IterationStatement
//    | ContinueStatement
//    | BreakStatement
//    | ReturnStatement
//    | WithStatement
//    | LabelledStatement
//    | SwitchStatement
//    | ThrowStatement
//    | TryStatement
//    | DebuggerStatement

// Expression
template<typename Factory>
class Inherit<Factory, kExpression>
  : public AstNode<Factory> {
};
INHERIT(Expression);

template<typename Factory>
class Expression : public ExpressionBase<Factory> {
 public:
  inline virtual bool IsValidLeftHandSide() const { return false; }
  DECLARE_NODE_TYPE(Expression)
};

// Literal
template<typename Factory>
class Inherit<Factory, kLiteral>
  : public Expression<Factory> {
};
INHERIT(Literal);

template<typename Factory>
class Literal : public LiteralBase<Factory> {
 public:
  DECLARE_NODE_TYPE(Literal)
};


// Statement
template<typename Factory>
class Inherit<Factory, kStatement>
  : public AstNode<Factory> {
};
INHERIT(Statement);

template<typename Factory>
class Statement : public StatementBase<Factory> {
 public:
  DECLARE_NODE_TYPE(Statement)
};

// BreakableStatement
template<typename Factory>
class Inherit<Factory, kBreakableStatement>
  : public Statement<Factory> {
};
INHERIT(BreakableStatement);

template<typename Factory>
class BreakableStatement : public BreakableStatementBase<Factory> {
 public:
  DECLARE_NODE_TYPE(BreakableStatement)
};

// NamedOnlyBreakableStatement
template<typename Factory>
class Inherit<Factory, kNamedOnlyBreakableStatement>
  : public BreakableStatement<Factory> {
};
INHERIT(NamedOnlyBreakableStatement);

template<typename Factory>
class NamedOnlyBreakableStatement
  : public NamedOnlyBreakableStatementBase<Factory> {
 public:
  DECLARE_NODE_TYPE(NamedOnlyBreakableStatement)
};

// AnonymousBreakableStatement
template<typename Factory>
class Inherit<Factory, kAnonymousBreakableStatement>
  : public BreakableStatement<Factory> {
};
INHERIT(AnonymousBreakableStatement);

template<typename Factory>
class AnonymousBreakableStatement
  : public AnonymousBreakableStatementBase<Factory> {
 public:
  DECLARE_NODE_TYPE(AnonymousBreakableStatement)
};

// Block
template<typename Factory>
class Inherit<Factory, kBlock>
  : public NamedOnlyBreakableStatement<Factory> {
};
INHERIT(Block);

template<typename Factory>
class Block : public BlockBase<Factory> {
 public:
  typedef typename AstNode<Factory>::Statements Statements;
  explicit Block(Statements* body)
     : body_(body) { }
  inline const Statements& body() const {
    return *body_;
  }
  DECLARE_DERIVED_NODE_TYPE(Block)
 private:
  Statements* body_;
};

// FunctionStatement
template<typename Factory>
class Inherit<Factory, kFunctionStatement>
  : public Statement<Factory> {
};
INHERIT(FunctionStatement);

template<typename Factory>
class FunctionStatement : public FunctionStatementBase<Factory> {
 public:
  explicit FunctionStatement(FunctionLiteral<Factory>* func)
    : function_(func) {
  }
  inline FunctionLiteral<Factory>* function() const {
    return function_;
  }
  DECLARE_DERIVED_NODE_TYPE(FunctionStatement)
 private:
  FunctionLiteral<Factory>* function_;
};

// FunctionDeclaration
template<typename Factory>
class Inherit<Factory, kFunctionDeclaration>
  : public Statement<Factory> {
};
INHERIT(FunctionDeclaration);

template<typename Factory>
class FunctionDeclaration : public FunctionDeclarationBase<Factory> {
 public:
  explicit FunctionDeclaration(FunctionLiteral<Factory>* func)
    : function_(func) {
  }
  inline FunctionLiteral<Factory>* function() const {
    return function_;
  }
  DECLARE_DERIVED_NODE_TYPE(FunctionDeclaration)
 private:
  FunctionLiteral<Factory>* function_;
};

// VariableStatement
template<typename Factory>
class Inherit<Factory, kVariableStatement>
  : public Statement<Factory> {
};
INHERIT(VariableStatement);

template<typename Factory>
class VariableStatement : public VariableStatementBase<Factory> {
 public:
  typedef typename AstNode<Factory>::Declarations Declarations;
  VariableStatement(Token::Type type, Declarations* decls)
    : is_const_(type == Token::CONST),
      decls_(decls) { }
  inline const Declarations& decls() const {
    return *decls_;
  }
  inline bool IsConst() const {
    return is_const_;
  }
  DECLARE_DERIVED_NODE_TYPE(VariableStatement)
 private:
  const bool is_const_;
  Declarations* decls_;
};

// Declaration
template<typename Factory>
class Inherit<Factory, kDeclaration>
  : public AstNode<Factory> {
};
INHERIT(Declaration);

template<typename Factory>
class Declaration : public DeclarationBase<Factory> {
 public:
  Declaration(Identifier<Factory>* name,
              Maybe<Expression<Factory> > expr)
    : name_(name),
      expr_(expr) {
  }
  inline Identifier<Factory>* name() const {
    return name_;
  }
  inline Maybe<Expression<Factory> > expr() const {
    return expr_;
  }
  DECLARE_DERIVED_NODE_TYPE(Declaration)
 private:
  Identifier<Factory>* name_;
  Maybe<Expression<Factory> > expr_;
};

// EmptyStatement
template<typename Factory>
class Inherit<Factory, kEmptyStatement>
  : public Statement<Factory> {
};
INHERIT(EmptyStatement);

template<typename Factory>
class EmptyStatement : public EmptyStatementBase<Factory> {
 public:
  DECLARE_DERIVED_NODE_TYPE(EmptyStatement)
};

// IfStatement
template<typename Factory>
class Inherit<Factory, kIfStatement>
  : public Statement<Factory> {
};
INHERIT(IfStatement);

template<typename Factory>
class IfStatement : public IfStatementBase<Factory> {
 public:
  IfStatement(Expression<Factory>* cond,
              Statement<Factory>* then,
              Maybe<Statement<Factory> > elses)
    : cond_(cond),
      then_(then),
      else_(elses) {
  }
  inline Expression<Factory>* cond() const { return cond_; }
  inline Statement<Factory>* then_statement() const { return then_; }
  inline Maybe<Statement<Factory> > else_statement() const { return else_; }
  DECLARE_DERIVED_NODE_TYPE(IfStatement)
 private:
  Expression<Factory>* cond_;
  Statement<Factory>* then_;
  Maybe<Statement<Factory> > else_;
};

// IterationStatement
template<typename Factory>
class Inherit<Factory, kIterationStatement>
  : public AnonymousBreakableStatement<Factory> {
};
INHERIT(IterationStatement);

template<typename Factory>
class IterationStatement : public IterationStatementBase<Factory> {
 public:
  DECLARE_NODE_TYPE(IterationStatement)
};

// DoWhileStatement
template<typename Factory>
class Inherit<Factory, kDoWhileStatement>
  : public IterationStatement<Factory> {
};
INHERIT(DoWhileStatement);

template<typename Factory>
class DoWhileStatement : public DoWhileStatementBase<Factory> {
 public:
  DoWhileStatement(Statement<Factory>* body, Expression<Factory>* cond)
    : body_(body),
      cond_(cond) {
  }
  inline Statement<Factory>* body() const { return body_; }
  inline Expression<Factory>* cond() const { return cond_; }
  DECLARE_DERIVED_NODE_TYPE(DoWhileStatement)
 private:
  Statement<Factory>* body_;
  Expression<Factory>* cond_;
};

// WhileStatement
template<typename Factory>
class Inherit<Factory, kWhileStatement>
  : public IterationStatement<Factory> {
};
INHERIT(WhileStatement);

template<typename Factory>
class WhileStatement : public WhileStatementBase<Factory> {
 public:
  WhileStatement(Statement<Factory>* body, Expression<Factory>* cond)
    : body_(body),
      cond_(cond) {
  }
  inline Statement<Factory>* body() const { return body_; }
  inline Expression<Factory>* cond() const { return cond_; }
  DECLARE_DERIVED_NODE_TYPE(WhileStatement)
 private:
  Statement<Factory>* body_;
  Expression<Factory>* cond_;
};

// ForStatement
template<typename Factory>
class Inherit<Factory, kForStatement>
  : public IterationStatement<Factory> {
};
INHERIT(ForStatement);

template<typename Factory>
class ForStatement : public ForStatementBase<Factory> {
 public:
  ForStatement(Statement<Factory>* body,
               Maybe<Statement<Factory> > init,
               Maybe<Expression<Factory> > cond,
               Maybe<Statement<Factory> > next)
    : body_(body),
      init_(init),
      cond_(cond),
      next_(next) {
  }
  inline Statement<Factory>* body() const { return body_; }
  inline Maybe<Statement<Factory> > init() const { return init_; }
  inline Maybe<Expression<Factory> > cond() const { return cond_; }
  inline Maybe<Statement<Factory> > next() const { return next_; }
  DECLARE_DERIVED_NODE_TYPE(ForStatement)
 private:
  Statement<Factory>* body_;
  Maybe<Statement<Factory> > init_;
  Maybe<Expression<Factory> > cond_;
  Maybe<Statement<Factory> > next_;
};

// ForInStatement
template<typename Factory>
class Inherit<Factory, kForInStatement>
  : public IterationStatement<Factory> {
};
INHERIT(ForInStatement);

template<typename Factory>
class ForInStatement : public ForInStatementBase<Factory> {
 public:
  ForInStatement(Statement<Factory>* body,
                 Statement<Factory>* each,
                 Expression<Factory>* enumerable)
    : body_(body),
      each_(each),
      enumerable_(enumerable) {
  }
  inline Statement<Factory>* body() const { return body_; }
  inline Statement<Factory>* each() const { return each_; }
  inline Expression<Factory>* enumerable() const { return enumerable_; }
  DECLARE_DERIVED_NODE_TYPE(ForInStatement)
 private:
  Statement<Factory>* body_;
  Statement<Factory>* each_;
  Expression<Factory>* enumerable_;
};

// ContinueStatement
template<typename Factory>
class Inherit<Factory, kContinueStatement>
  : public Statement<Factory> {
};
INHERIT(ContinueStatement);

template<typename Factory>
class ContinueStatement : public ContinueStatementBase<Factory> {
 public:
  ContinueStatement(Maybe<Identifier<Factory> > label,
                    IterationStatement<Factory>** target)
    : label_(label),
      target_(target) {
    assert(target_);
  }
  inline Maybe<Identifier<Factory> > label() const { return label_; }
  inline IterationStatement<Factory>* target() const {
    assert(target_ && *target_);
    return *target_;
  }
  DECLARE_DERIVED_NODE_TYPE(ContinueStatement)
 private:
  Maybe<Identifier<Factory> > label_;
  IterationStatement<Factory>** target_;
};

// BreakStatement
template<typename Factory>
class Inherit<Factory, kBreakStatement>
  : public Statement<Factory> {
};
INHERIT(BreakStatement);

template<typename Factory>
class BreakStatement : public BreakStatementBase<Factory> {
 public:
  BreakStatement(Maybe<Identifier<Factory> > label,
                 BreakableStatement<Factory>** target)
    : label_(label),
      target_(target) {
    // example >
    //   do {
    //     test: break test;
    //   } while (0);
    // if above example, target is NULL
    assert(target_ || label_);
  }
  inline Maybe<Identifier<Factory> > label() const { return label_; }
  inline BreakableStatement<Factory>* target() const {
    if (target_) {
      assert(*target_);
      return *target_;
    } else {
      return NULL;
    }
  }
  DECLARE_DERIVED_NODE_TYPE(BreakStatement)
 private:
  Maybe<Identifier<Factory> > label_;
  BreakableStatement<Factory>** target_;
};

// ReturnStatement
template<typename Factory>
class Inherit<Factory, kReturnStatement>
  : public Statement<Factory> {
};
INHERIT(ReturnStatement);

template<typename Factory>
class ReturnStatement : public ReturnStatementBase<Factory> {
 public:
  explicit ReturnStatement(Maybe<Expression<Factory> > expr)
    : expr_(expr) {
  }
  inline Maybe<Expression<Factory> > expr() const { return expr_; }
  DECLARE_DERIVED_NODE_TYPE(ReturnStatement)
 private:
  Maybe<Expression<Factory> > expr_;
};

// WithStatement
template<typename Factory>
class Inherit<Factory, kWithStatement>
  : public Statement<Factory> {
};
INHERIT(WithStatement);

template<typename Factory>
class WithStatement : public WithStatementBase<Factory> {
 public:
  WithStatement(Expression<Factory>* context,
                Statement<Factory>* body)
    : context_(context),
      body_(body) {
  }
  inline Expression<Factory>* context() const { return context_; }
  inline Statement<Factory>* body() const { return body_; }
  DECLARE_DERIVED_NODE_TYPE(WithStatement)
 private:
  Expression<Factory>* context_;
  Statement<Factory>* body_;
};

// LabelledStatement
template<typename Factory>
class Inherit<Factory, kLabelledStatement>
  : public Statement<Factory> {
};
INHERIT(LabelledStatement);

template<typename Factory>
class LabelledStatement : public LabelledStatementBase<Factory> {
 public:
  LabelledStatement(Expression<Factory>* expr, Statement<Factory>* body)
    : body_(body) {
    label_ = expr->AsLiteral()->AsIdentifier();
  }
  inline Identifier<Factory>* label() const { return label_; }
  inline Statement<Factory>* body() const { return body_; }
  DECLARE_DERIVED_NODE_TYPE(LabelledStatement)
 private:
  Identifier<Factory>* label_;
  Statement<Factory>* body_;
};

// CaseClause
template<typename Factory>
class Inherit<Factory, kCaseClause> : public AstNode<Factory> {
};
INHERIT(CaseClause);

template<typename Factory>
class CaseClause : public CaseClauseBase<Factory> {
 public:
  typedef typename AstNode<Factory>::Statements Statements;
  explicit CaseClause(bool is_default,
                      Maybe<Expression<Factory> > expr,
                      Statements* body)
    : expr_(expr),
      body_(body),
      default_(is_default) {
  }
  inline bool IsDefault() const {
    return !expr_;
  }
  inline Maybe<Expression<Factory> > expr() const {
    return expr_;
  }
  inline const Statements& body() const {
    return *body_;
  }
  DECLARE_DERIVED_NODE_TYPE(CaseClause)
 private:
  Maybe<Expression<Factory> > expr_;
  Statements* body_;
  bool default_;
};

// SwitchStatement
template<typename Factory>
class Inherit<Factory, kSwitchStatement>
  : public AnonymousBreakableStatement<Factory> {
};
INHERIT(SwitchStatement);

template<typename Factory>
class SwitchStatement : public SwitchStatementBase<Factory> {
 public:
  typedef typename AstNode<Factory>::CaseClauses CaseClauses;
  SwitchStatement(Expression<Factory>* expr,
                  CaseClauses* clauses)
    : expr_(expr),
      clauses_(clauses) {
  }
  inline Expression<Factory>* expr() const { return expr_; }
  inline const CaseClauses& clauses() const { return *clauses_; }
  DECLARE_DERIVED_NODE_TYPE(SwitchStatement)
 private:
  Expression<Factory>* expr_;
  CaseClauses* clauses_;
};

// ThrowStatement
template<typename Factory>
class Inherit<Factory, kThrowStatement>
  : public Statement<Factory> {
};
INHERIT(ThrowStatement);

template<typename Factory>
class ThrowStatement : public ThrowStatementBase<Factory> {
 public:
  explicit ThrowStatement(Expression<Factory>* expr)
    : expr_(expr) {
  }
  inline Expression<Factory>* expr() const { return expr_; }
  DECLARE_DERIVED_NODE_TYPE(ThrowStatement)
 private:
  Expression<Factory>* expr_;
};

// TryStatement
template<typename Factory>
class Inherit<Factory, kTryStatement>
  : public Statement<Factory> {
};
INHERIT(TryStatement);

template<typename Factory>
class TryStatement : public TryStatementBase<Factory> {
 public:
  explicit TryStatement(Block<Factory>* try_block,
                        Maybe<Identifier<Factory> > catch_name,
                        Maybe<Block<Factory> > catch_block,
                        Maybe<Block<Factory> > finally_block)
    : body_(try_block),
      catch_name_(catch_name),
      catch_block_(catch_block),
      finally_block_(finally_block) {
  }
  inline Block<Factory>* body() const { return body_; }
  inline Maybe<Identifier<Factory> > catch_name() const { return catch_name_; }
  inline Maybe<Block<Factory> > catch_block() const { return catch_block_; }
  inline Maybe<Block<Factory> > finally_block() const { return finally_block_; }
  DECLARE_DERIVED_NODE_TYPE(TryStatement)
 private:
  Block<Factory>* body_;
  Maybe<Identifier<Factory> > catch_name_;
  Maybe<Block<Factory> > catch_block_;
  Maybe<Block<Factory> > finally_block_;
};

// DebuggerStatement
template<typename Factory>
class Inherit<Factory, kDebuggerStatement>
  : public Statement<Factory> {
};
INHERIT(DebuggerStatement);

template<typename Factory>
class DebuggerStatement : public DebuggerStatementBase<Factory> {
  DECLARE_DERIVED_NODE_TYPE(DebuggerStatement)
};

// ExpressionStatement
template<typename Factory>
class Inherit<Factory, kExpressionStatement>
  : public Statement<Factory> {
};
INHERIT(ExpressionStatement);

template<typename Factory>
class ExpressionStatement : public ExpressionStatementBase<Factory> {
 public:
  explicit ExpressionStatement(Expression<Factory>* expr) : expr_(expr) { }
  inline Expression<Factory>* expr() const { return expr_; }
  DECLARE_DERIVED_NODE_TYPE(ExpressionStatement)
 private:
  Expression<Factory>* expr_;
};

// Assignment
template<typename Factory>
class Inherit<Factory, kAssignment>
  : public Expression<Factory> {
};
INHERIT(Assignment);

template<typename Factory>
class Assignment : public AssignmentBase<Factory> {
 public:
  Assignment(Token::Type op,
             Expression<Factory>* left, Expression<Factory>* right)
    : op_(op),
      left_(left),
      right_(right) {
  }
  inline Token::Type op() const { return op_; }
  inline Expression<Factory>* left() const { return left_; }
  inline Expression<Factory>* right() const { return right_; }
  DECLARE_DERIVED_NODE_TYPE(Assignment)
 private:
  Token::Type op_;
  Expression<Factory>* left_;
  Expression<Factory>* right_;
};

// BinaryOperation
template<typename Factory>
class Inherit<Factory, kBinaryOperation>
  : public Expression<Factory> {
};
INHERIT(BinaryOperation);

template<typename Factory>
class BinaryOperation : public BinaryOperationBase<Factory> {
 public:
  BinaryOperation(Token::Type op,
                  Expression<Factory>* left, Expression<Factory>* right)
    : op_(op),
      left_(left),
      right_(right) {
  }
  inline Token::Type op() const { return op_; }
  inline Expression<Factory>* left() const { return left_; }
  inline Expression<Factory>* right() const { return right_; }
  DECLARE_DERIVED_NODE_TYPE(BinaryOperation)
 private:
  Token::Type op_;
  Expression<Factory>* left_;
  Expression<Factory>* right_;
};

// ConditionalExpression
template<typename Factory>
class Inherit<Factory, kConditionalExpression>
  : public Expression<Factory> {
};
INHERIT(ConditionalExpression);

template<typename Factory>
class ConditionalExpression : public ConditionalExpressionBase<Factory> {
 public:
  ConditionalExpression(Expression<Factory>* cond,
                        Expression<Factory>* left,
                        Expression<Factory>* right)
    : cond_(cond), left_(left), right_(right) {
  }
  inline Expression<Factory>* cond() const { return cond_; }
  inline Expression<Factory>* left() const { return left_; }
  inline Expression<Factory>* right() const { return right_; }
  DECLARE_DERIVED_NODE_TYPE(ConditionalExpression)
 private:
  Expression<Factory>* cond_;
  Expression<Factory>* left_;
  Expression<Factory>* right_;
};

// UnaryOperation
template<typename Factory>
class Inherit<Factory, kUnaryOperation>
  : public Expression<Factory> {
};
INHERIT(UnaryOperation);

template<typename Factory>
class UnaryOperation : public UnaryOperationBase<Factory> {
 public:
  UnaryOperation(Token::Type op, Expression<Factory>* expr)
    : op_(op),
      expr_(expr) {
  }
  inline Token::Type op() const { return op_; }
  inline Expression<Factory>* expr() const { return expr_; }
  DECLARE_DERIVED_NODE_TYPE(UnaryOperation)
 private:
  Token::Type op_;
  Expression<Factory>* expr_;
};

// PostfixExpression
template<typename Factory>
class Inherit<Factory, kPostfixExpression>
  : public Expression<Factory> {
};
INHERIT(PostfixExpression);

template<typename Factory>
class PostfixExpression : public PostfixExpressionBase<Factory> {
 public:
  PostfixExpression(Token::Type op, Expression<Factory>* expr)
    : op_(op),
      expr_(expr) {
  }
  inline Token::Type op() const { return op_; }
  inline Expression<Factory>* expr() const { return expr_; }
  DECLARE_DERIVED_NODE_TYPE(PostfixExpression)
 private:
  Token::Type op_;
  Expression<Factory>* expr_;
};

// StringLiteral
template<typename Factory>
class Inherit<Factory, kStringLiteral>
  : public Literal<Factory> {
};
INHERIT(StringLiteral);

template<typename Factory>
class StringLiteral : public StringLiteralBase<Factory> {
 public:
  typedef typename SpaceUString<Factory>::type value_type;
  StringLiteral(const std::vector<uc16>& buffer,
                Factory* factory) {
    InitializeStringLiteral(buffer, factory);
  }

  inline const value_type& value() const {
    return value_;
  }
  DECLARE_DERIVED_NODE_TYPE(StringLiteral)
 protected:
  StringLiteral() { }
  void InitializeStringLiteral(const std::vector<uc16>& buffer,
                               Factory* factory) {
    value_ = value_type(
        buffer.data(),
        buffer.size(),
        typename value_type::allocator_type(factory));
  }
 private:
  value_type value_;
};

// Directivable
template<typename Factory>
class Inherit<Factory, kDirectivable>
  : public StringLiteral<Factory> {
};
INHERIT(Directivable);

template<typename Factory>
class Directivable : public DirectivableBase<Factory> {
 public:
  explicit Directivable(const std::vector<uc16>& buffer,
                        Factory* factory) {
    InitializeStringLiteral(buffer, factory);
  }
  DECLARE_NODE_TYPE(Directivable)
};

// NumberLiteral
template<typename Factory>
class Inherit<Factory, kNumberLiteral>
  : public Literal<Factory> {
};
INHERIT(NumberLiteral);

template<typename Factory>
class NumberLiteral : public NumberLiteralBase<Factory> {
 public:
  explicit NumberLiteral(const double & val)
    : value_(val) {
  }
  inline double value() const { return value_; }
  DECLARE_DERIVED_NODE_TYPE(NumberLiteral)
 private:
  double value_;
};

// Identifier
template<typename Factory>
class Inherit<Factory, kIdentifier>
  : public Literal<Factory> {
};
INHERIT(Identifier);

template<typename Factory>
class Identifier : public IdentifierBase<Factory> {
 public:
  typedef typename SpaceUString<Factory>::type value_type;
  template<typename Range>
  Identifier(const Range& range, Factory* factory)
    : value_(range.begin(),
             range.end(),
             typename value_type::allocator_type(factory)) {
  }
  inline const value_type& value() const {
    return value_;
  }
  inline bool IsValidLeftHandSide() const { return true; }
  DECLARE_DERIVED_NODE_TYPE(Identifier)
 protected:
  value_type value_;
};

template<typename Factory>
class IdentifierKey {
 public:
  typedef Identifier<Factory> value_type;
  typedef IdentifierKey<Factory> this_type;
  IdentifierKey(value_type* ident)  // NOLINT
    : ident_(ident) { }
  IdentifierKey(const this_type& rhs)  // NOLINT
    : ident_(rhs.ident_) { }
  inline const typename value_type::value_type& value() const {
    return ident_->value();
  }
  inline bool operator==(const this_type& rhs) const {
    if (ident_ == rhs.ident_) {
      return true;
    } else {
      return value() == rhs.value();
    }
  }
  inline bool operator>(const this_type& rhs) const {
    return value() > rhs.value();
  }
  inline bool operator<(const this_type& rhs) const {
    return value() < rhs.value();
  }
  inline bool operator>=(const this_type& rhs) const {
    return value() >= rhs.value();
  }
  inline bool operator<=(const this_type& rhs) const {
    return value() <= rhs.value();
  }
 private:
  value_type* ident_;
};

// ThisLiteral
template<typename Factory>
class Inherit<Factory, kThisLiteral>
  : public Literal<Factory> {
};
INHERIT(ThisLiteral);

template<typename Factory>
class ThisLiteral : public ThisLiteralBase<Factory> {
 public:
  DECLARE_DERIVED_NODE_TYPE(ThisLiteral)
};

// NullLiteral
template<typename Factory>
class Inherit<Factory, kNullLiteral>
  : public Literal<Factory> {
};
INHERIT(NullLiteral);

template<typename Factory>
class NullLiteral : public NullLiteralBase<Factory> {
 public:
  DECLARE_DERIVED_NODE_TYPE(NullLiteral)
};

// TrueLiteral
template<typename Factory>
class Inherit<Factory, kTrueLiteral>
  : public Literal<Factory> {
};
INHERIT(TrueLiteral);

template<typename Factory>
class TrueLiteral : public TrueLiteralBase<Factory> {
 public:
  DECLARE_DERIVED_NODE_TYPE(TrueLiteral)
};

// FalseLiteral
template<typename Factory>
class Inherit<Factory, kFalseLiteral>
  : public Literal<Factory> {
};
INHERIT(FalseLiteral);

template<typename Factory>
class FalseLiteral : public FalseLiteralBase<Factory> {
 public:
  DECLARE_DERIVED_NODE_TYPE(FalseLiteral)
};

// RegExpLiteral
template<typename Factory>
class Inherit<Factory, kRegExpLiteral>
  : public Literal<Factory> {
};
INHERIT(RegExpLiteral);

template<typename Factory>
class RegExpLiteral : public RegExpLiteralBase<Factory> {
 public:
  typedef typename SpaceUString<Factory>::type value_type;

  RegExpLiteral(const std::vector<uc16>& buffer,
                const std::vector<uc16>& flags,
                Factory* factory)
    : value_(buffer.data(),
             buffer.size(), typename value_type::allocator_type(factory)),
      flags_(flags.data(),
             flags.size(), typename value_type::allocator_type(factory)) {
  }
  inline const value_type& value() const { return value_; }
  inline const value_type& flags() const { return flags_; }
  DECLARE_DERIVED_NODE_TYPE(RegExpLiteral)
 protected:
  value_type value_;
  value_type flags_;
};

// ArrayLiteral
template<typename Factory>
class Inherit<Factory, kArrayLiteral>
  : public Literal<Factory> {
};
INHERIT(ArrayLiteral);


// items is NULL able
template<typename Factory>
class ArrayLiteral : public ArrayLiteralBase<Factory> {
 public:
  typedef typename AstNode<Factory>::MaybeExpressions MaybeExpressions;

  explicit ArrayLiteral(MaybeExpressions* items)
    : items_(items) {
  }
  inline const MaybeExpressions& items() const {
    return *items_;
  }
  DECLARE_DERIVED_NODE_TYPE(ArrayLiteral)
 private:
  MaybeExpressions* items_;
};

// ObjectLiteral
template<typename Factory>
class Inherit<Factory, kObjectLiteral>
  : public Literal<Factory> {
};
INHERIT(ObjectLiteral);

template<typename Factory>
class ObjectLiteral : public ObjectLiteralBase<Factory> {
 public:
  enum PropertyDescriptorType {
    DATA = 1,
    GET  = 2,
    SET  = 4
  };
  typedef std::tr1::tuple<PropertyDescriptorType,
                          Identifier<Factory>*,
                          Expression<Factory>*> Property;
  typedef typename SpaceVector<Factory, Property>::type Properties;
  explicit ObjectLiteral(Properties* properties)
    : properties_(properties) {
  }

  static inline void AddDataProperty(Properties* prop,
                                     Identifier<Factory>* key,
                                     Expression<Factory>* val) {
    prop->push_back(std::tr1::make_tuple(DATA, key, val));
  }

  static inline void AddAccessor(Properties* prop,
                                 PropertyDescriptorType type,
                                 Identifier<Factory>* key,
                                 Expression<Factory>* val) {
    prop->push_back(std::tr1::make_tuple(type, key, val));
  }

  inline const Properties& properties() const {
    return *properties_;
  }
  DECLARE_DERIVED_NODE_TYPE(ObjectLiteral)

 private:
  Properties* properties_;
};

// FunctionLiteral
template<typename Factory>
class Inherit<Factory, kFunctionLiteral>
  : public Literal<Factory> {
};
INHERIT(FunctionLiteral);

template<typename Factory>
class FunctionLiteral : public FunctionLiteralBase<Factory> {
 public:
  typedef typename AstNode<Factory>::Statements Statements;
  typedef typename AstNode<Factory>::Identifiers Identifiers;
  enum DeclType {
    DECLARATION,
    STATEMENT,
    EXPRESSION,
    GLOBAL
  };
  enum ArgType {
    GENERAL,
    SETTER,
    GETTER
  };
  inline Maybe<Identifier<Factory> > name() const {
    return name_;
  }
  inline DeclType type() const { return type_; }
  inline const Identifiers& params() const {
    return *params_;
  }
  inline const Statements& body() const {
    return *body_;
  }
  inline const Scope<Factory>& scope() const {
    return *scope_;
  }
  inline std::size_t start_position() const {
    return start_position_;
  }
  inline std::size_t end_position() const {
    return end_position_;
  }
  inline bool strict() const {
    return strict_;
  }
  FunctionLiteral(DeclType type,
                  Maybe<Identifier<Factory> > name,
                  Identifiers* params,
                  Statements* body,
                  Scope<Factory>* scope,
                  bool strict,
                  std::size_t start_position,
                  std::size_t end_position)
    : name_(name),
      type_(type),
      params_(params),
      body_(body),
      scope_(scope),
      strict_(strict),
      start_position_(start_position),
      end_position_(end_position) {
  }
  DECLARE_DERIVED_NODE_TYPE(FunctionLiteral)
 private:
  Maybe<Identifier<Factory> > name_;
  DeclType type_;
  Identifiers* params_;
  Statements* body_;
  Scope<Factory>* scope_;
  bool strict_;
  std::size_t start_position_;
  std::size_t end_position_;
};

// PropertyAccess
template<typename Factory>
class Inherit<Factory, kPropertyAccess>
  : public Expression<Factory> {
};
INHERIT(PropertyAccess);

template<typename Factory>
class PropertyAccess : public PropertyAccessBase<Factory> {
 public:
  inline bool IsValidLeftHandSide() const { return true; }
  inline Expression<Factory>* target() const { return target_; }
  DECLARE_NODE_TYPE(PropertyAccess)
 protected:
  void InitializePropertyAccess(Expression<Factory>* obj) {
    target_ = obj;
  }
  Expression<Factory>* target_;
};

// IdentifierAccess
template<typename Factory>
class Inherit<Factory, kIdentifierAccess>
  : public PropertyAccess<Factory> {
};
INHERIT(IdentifierAccess);

template<typename Factory>
class IdentifierAccess : public IdentifierAccessBase<Factory> {
 public:
  IdentifierAccess(Expression<Factory>* obj,
                   Identifier<Factory>* key)
    : key_(key) {
    InitializePropertyAccess(obj);
  }
  inline Identifier<Factory>* key() const { return key_; }
  DECLARE_DERIVED_NODE_TYPE(IdentifierAccess)
 private:
  Identifier<Factory>* key_;
};

// IndexAccess
template<typename Factory>
class Inherit<Factory, kIndexAccess>
  : public PropertyAccess<Factory> {
};
INHERIT(IndexAccess);

template<typename Factory>
class IndexAccess : public IndexAccessBase<Factory> {
 public:
  IndexAccess(Expression<Factory>* obj,
              Expression<Factory>* key)
    : key_(key) {
    InitializePropertyAccess(obj);
  }
  inline Expression<Factory>* key() const { return key_; }
  DECLARE_DERIVED_NODE_TYPE(IndexAccess)
 private:
  Expression<Factory>* key_;
};

// Call
template<typename Factory>
class Inherit<Factory, kCall>
  : public Expression<Factory> {
};
INHERIT(Call);

template<typename Factory>
class Call : public CallBase<Factory> {
 public:
  inline bool IsValidLeftHandSide() const { return true; }
  DECLARE_NODE_TYPE(Call)
};

// FunctionCall
template<typename Factory>
class Inherit<Factory, kFunctionCall>
  : public Call<Factory> {
};
INHERIT(FunctionCall);

template<typename Factory>
class FunctionCall : public FunctionCallBase<Factory> {
 public:
  typedef typename AstNode<Factory>::Expressions Expressions;
  FunctionCall(Expression<Factory>* target,
               Expressions* args)
    : target_(target),
      args_(args) {
  }
  inline Expression<Factory>* target() const { return target_; }
  inline const Expressions& args() const { return *args_; }
  DECLARE_DERIVED_NODE_TYPE(FunctionCall)
 private:
  Expression<Factory>* target_;
  Expressions* args_;
};

// ConstructorCall
template<typename Factory>
class Inherit<Factory, kConstructorCall>
  : public Call<Factory> {
};
INHERIT(ConstructorCall);

template<typename Factory>
class ConstructorCall : public ConstructorCallBase<Factory> {
 public:
  typedef typename AstNode<Factory>::Expressions Expressions;
  ConstructorCall(Expression<Factory>* target,
                  Expressions* args)
    : target_(target),
      args_(args) {
  }
  inline Expression<Factory>* target() const { return target_; }
  inline const Expressions& args() const { return *args_; }
  DECLARE_DERIVED_NODE_TYPE(ConstructorCall)
 private:
  Expression<Factory>* target_;
  Expressions* args_;
};

} } }  // namespace iv::core::ast
namespace std {
namespace tr1 {
// template specialization
// for iv::core::Parser::IdentifierWrapper in std::tr1::unordered_map
// allowed in section 17.4.3.1
template<class Factory>
struct hash<iv::core::ast::IdentifierKey<Factory> >
  : public unary_function<iv::core::ast::IdentifierKey<Factory>, std::size_t> {
  std::size_t operator()(const iv::core::ast::IdentifierKey<Factory>& x) const {
    return hash<
        typename iv::core::ast::Identifier<Factory>::value_type>()(x.value());
  }
};
} }  // namespace std::tr1
#endif  // _IV_AST_H_
