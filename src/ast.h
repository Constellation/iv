#ifndef _IV_AST_H_
#define _IV_AST_H_
#include <vector>
#include <tr1/unordered_map>
#include <tr1/tuple>
#include "uchar.h"
#include "noncopyable.h"
#include "utils.h"
#include "alloc-inl.h"
#include "functor.h"
#include "token.h"
#include "scope.h"
#include "ast-visitor.h"
#include "source.h"

namespace iv {
namespace core {
#define ACCEPT_VISITOR \
  inline void Accept(AstVisitor* visitor) {\
    visitor->Visit(this);\
  }\
  inline void Accept(ConstAstVisitor* visitor) const {\
    visitor->Visit(this);\
  }

#define DECLARE_NODE_TYPE(type) \
  inline const type* As##type() const { return this; }\
  inline type* As##type() { return this; }

#define DECLARE_NODE_TYPE_BASE(type) \
  inline virtual const type* As##type() const { return NULL; }\
  inline virtual type* As##type() { return NULL; }

// forward declarations
class Statement;
class ExpressionStatement;
class EmptyStatement;
class DebuggerStatement;
class FunctionStatement;
class IfStatement;
class VariableStatement;
class IterationStatement;
class DoWhileStatement;
class WhileStatement;
class ForStatement;
class ForInStatement;
class ContinueStatement;
class BreakStatement;
class ReturnStatement;
class WithStatement;
class SwitchStatement;
class LabelledStatement;
class ThrowStatement;
class TryStatement;
class BreakableStatement;
class NamedOnlyBreakableStatement;
class AnonymousBreakableStatement;

class Expression;
class Literal;
class FunctionLiteral;
class Assignment;
class UnaryOperation;
class BinaryOperation;
class ConditionalExpression;
class PropertyAccess;
class IdentifierAccess;
class IndexAccess;
class Call;
class ConstructorCall;
class FunctionCall;
class Identifier;
class Declaration;

class ThisLiteral;
class NullLiteral;
class TrueLiteral;
class FalseLiteral;
class NumberLiteral;
class StringLiteral;
class Directivable;
class RegExpLiteral;
class ArrayLiteral;
class ObjectLiteral;
class FunctionLiteral;
class Undefined;

class AstNode : public SpaceObject, private Noncopyable<AstNode>::type {
 public:
  template<class T>
  struct List {
    typedef typename SpaceVector<T>::type type;
  };
  typedef List<Expression*>::type Expressions;
  typedef List<Statement*>::type Statements;
  typedef List<Declaration*>::type Declarations;
  typedef List<Identifier*>::type Identifiers;

  virtual ~AstNode() = 0;

  DECLARE_NODE_TYPE_BASE(Statement)
  DECLARE_NODE_TYPE_BASE(Block)
  DECLARE_NODE_TYPE_BASE(ExpressionStatement)
  DECLARE_NODE_TYPE_BASE(EmptyStatement)
  DECLARE_NODE_TYPE_BASE(VariableStatement)
  DECLARE_NODE_TYPE_BASE(DebuggerStatement)
  DECLARE_NODE_TYPE_BASE(FunctionStatement)
  DECLARE_NODE_TYPE_BASE(IfStatement)
  DECLARE_NODE_TYPE_BASE(IterationStatement)
  DECLARE_NODE_TYPE_BASE(DoWhileStatement)
  DECLARE_NODE_TYPE_BASE(WhileStatement)
  DECLARE_NODE_TYPE_BASE(ForStatement)
  DECLARE_NODE_TYPE_BASE(ForInStatement)
  DECLARE_NODE_TYPE_BASE(ContinueStatement)
  DECLARE_NODE_TYPE_BASE(BreakStatement)
  DECLARE_NODE_TYPE_BASE(ReturnStatement)
  DECLARE_NODE_TYPE_BASE(WithStatement)
  DECLARE_NODE_TYPE_BASE(LabelledStatement)
  DECLARE_NODE_TYPE_BASE(SwitchStatement)
  DECLARE_NODE_TYPE_BASE(ThrowStatement)
  DECLARE_NODE_TYPE_BASE(TryStatement)
  DECLARE_NODE_TYPE_BASE(BreakableStatement)
  DECLARE_NODE_TYPE_BASE(NamedOnlyBreakableStatement)
  DECLARE_NODE_TYPE_BASE(AnonymousBreakableStatement)

  DECLARE_NODE_TYPE_BASE(Expression)

  DECLARE_NODE_TYPE_BASE(Literal)
  DECLARE_NODE_TYPE_BASE(ThisLiteral)
  DECLARE_NODE_TYPE_BASE(NullLiteral)
  DECLARE_NODE_TYPE_BASE(FalseLiteral)
  DECLARE_NODE_TYPE_BASE(TrueLiteral)
  DECLARE_NODE_TYPE_BASE(Undefined)
  DECLARE_NODE_TYPE_BASE(NumberLiteral)
  DECLARE_NODE_TYPE_BASE(StringLiteral)
  DECLARE_NODE_TYPE_BASE(Directivable)
  DECLARE_NODE_TYPE_BASE(Identifier)
  DECLARE_NODE_TYPE_BASE(RegExpLiteral)
  DECLARE_NODE_TYPE_BASE(ArrayLiteral)
  DECLARE_NODE_TYPE_BASE(ObjectLiteral)
  DECLARE_NODE_TYPE_BASE(FunctionLiteral)

  DECLARE_NODE_TYPE_BASE(UnaryOperation)
  DECLARE_NODE_TYPE_BASE(PostfixExpression)
  DECLARE_NODE_TYPE_BASE(Assignment)
  DECLARE_NODE_TYPE_BASE(BinaryOperation)
  DECLARE_NODE_TYPE_BASE(ConditionalExpression)
  DECLARE_NODE_TYPE_BASE(PropertyAccess)
  DECLARE_NODE_TYPE_BASE(IdentifierAccess)
  DECLARE_NODE_TYPE_BASE(IndexAccess)
  DECLARE_NODE_TYPE_BASE(Call)
  DECLARE_NODE_TYPE_BASE(FunctionCall)
  DECLARE_NODE_TYPE_BASE(ConstructorCall)

  virtual void Accept(AstVisitor* visitor) = 0;
  virtual void Accept(ConstAstVisitor* visitor) const = 0;
};

inline AstNode::~AstNode() { }

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
class Expression : public AstNode {
 public:
  inline virtual bool IsValidLeftHandSide() const { return false; }
  DECLARE_NODE_TYPE(Expression)
};

class Literal : public Expression {
 public:
  DECLARE_NODE_TYPE(Literal)
};

class Statement : public AstNode {
 public:
  DECLARE_NODE_TYPE(Statement)
};

class BreakableStatement : public Statement {
 public:
  BreakableStatement() : labels_(NULL) { }
  void set_labels(Identifiers* labels) {
    labels_ = labels;
  }
  Identifiers* labels() const {
    return labels_;
  }
  DECLARE_NODE_TYPE(BreakableStatement)
 protected:
  Identifiers* labels_;
};

class NamedOnlyBreakableStatement : public BreakableStatement {
 public:
  DECLARE_NODE_TYPE(NamedOnlyBreakableStatement)
};

class AnonymousBreakableStatement : public BreakableStatement {
 public:
  DECLARE_NODE_TYPE(AnonymousBreakableStatement)
};

class Block : public NamedOnlyBreakableStatement {
 public:
  explicit Block(Space* factory)
     : body_(Statements::allocator_type(factory)) { }

  void AddStatement(Statement* stmt) {
    body_.push_back(stmt);
  }

  inline const Statements& body() const {
    return body_;
  }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(Block)
 private:
  Statements body_;
};

class FunctionStatement : public Statement {
 public:
  explicit FunctionStatement(FunctionLiteral* func)
    : function_(func) {
  }
  inline FunctionLiteral* function() const {
    return function_;
  }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(FunctionStatement)
 private:
  FunctionLiteral* function_;
};

class VariableStatement : public Statement {
 public:
  VariableStatement(Token::Type type, Space* factory)
    : is_const_(type == Token::CONST),
      decls_(Declarations::allocator_type(factory)) { }

  void AddDeclaration(Declaration* decl) {
    decls_.push_back(decl);
  }

  inline const Declarations& decls() const {
    return decls_;
  }
  inline bool IsConst() const {
    return is_const_;
  }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(VariableStatement)
 private:
  const bool is_const_;
  Declarations decls_;
};

class Declaration : public AstNode {
 public:
  Declaration(Identifier* name, Expression* expr)
    : name_(name),
      expr_(expr) {
  }
  inline Identifier* name() const {
    return name_;
  }
  inline Expression* expr() const {
    return expr_;
  }
  ACCEPT_VISITOR
 private:
  Identifier* name_;
  Expression* expr_;
};

class EmptyStatement : public Statement {
 public:
  DECLARE_NODE_TYPE(EmptyStatement)
  ACCEPT_VISITOR
};

class IfStatement : public Statement {
 public:
  IfStatement(Expression* cond, Statement* then)
    : cond_(cond),
      then_(then),
      else_(NULL) {
  }
  void SetElse(Statement* stmt) {
    else_ = stmt;
  }
  inline Expression* cond() const { return cond_; }
  inline Statement* then_statement() const { return then_; }
  inline Statement* else_statement() const { return else_; }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(IfStatement)
 private:
  Expression* cond_;
  Statement* then_;
  Statement* else_;
};

class IterationStatement : public AnonymousBreakableStatement {
 public:
  IterationStatement()
    : body_(NULL) {
  }
  inline Statement* body() const { return body_; }
  inline void set_body(Statement* stmt) {
    body_ = stmt;
  }
  DECLARE_NODE_TYPE(IterationStatement)
 private:
  Statement* body_;
};

class DoWhileStatement : public IterationStatement {
 public:
  DoWhileStatement()
    : IterationStatement(),
      cond_(NULL) {
  }
  inline Expression* cond() const { return cond_; }
  inline void set_cond(Expression* expr) {
    cond_ = expr;
  }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(DoWhileStatement)
 private:
  Expression* cond_;
};

class WhileStatement : public IterationStatement {
 public:
  explicit WhileStatement(Expression* cond)
    : IterationStatement(),
      cond_(cond) {
  }
  inline Expression* cond() const { return cond_; }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(WhileStatement)
 private:
  Expression* cond_;
};

class ForStatement : public IterationStatement {
 public:
  ForStatement()
    : IterationStatement(),
      init_(NULL),
      cond_(NULL),
      next_(NULL) {
  }
  inline void SetInit(Statement* init) { init_ = init; }
  inline void SetCondition(Expression* cond) { cond_ = cond; }
  inline void SetNext(Statement* next) { next_ = next; }
  inline Statement* init() const { return init_; }
  inline Expression* cond() const { return cond_; }
  inline Statement* next() const { return next_; }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(ForStatement)
 private:
  Statement* init_;
  Expression* cond_;
  Statement* next_;
};

class ForInStatement : public IterationStatement {
 public:
  ForInStatement(Statement* each,
                 Expression* enumerable)
    : IterationStatement(),
      each_(each),
      enumerable_(enumerable) {
  }
  inline Statement* each() const { return each_; }
  inline Expression* enumerable() const { return enumerable_; }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(ForInStatement)
 private:
  Statement* each_;
  Expression* enumerable_;
};

class ContinueStatement : public Statement {
 public:
  ContinueStatement()
    : label_(NULL) {
  }

  void SetLabel(Identifier* label) {
    label_ = label;
  }

  void SetTarget(IterationStatement* target) {
    target_ = target;
  }
  inline Identifier* label() const { return label_; }
  inline IterationStatement* target() const { return target_; }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(ContinueStatement)
 private:
  Identifier* label_;
  IterationStatement* target_;
};

class BreakStatement : public Statement {
 public:
  BreakStatement()
    : label_(NULL),
      target_(NULL) {
  }

  void SetLabel(Identifier* label) {
    label_ = label;
  }

  void SetTarget(BreakableStatement* target) {
    target_ = target;
  }
  inline Identifier* label() const { return label_; }
  inline BreakableStatement* target() const { return target_; }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(BreakStatement)
 private:
  Identifier* label_;
  BreakableStatement* target_;
};

class ReturnStatement : public Statement {
 public:
  explicit ReturnStatement(Expression* expr)
    : expr_(expr) {
  }
  inline Expression* expr() const { return expr_; }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(ReturnStatement)
 private:
  Expression* expr_;
};

class WithStatement : public Statement {
 public:
  WithStatement(Expression* context, Statement* body)
    : context_(context),
      body_(body) {
  }
  inline Expression* context() const { return context_; }
  inline Statement* body() const { return body_; }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(WithStatement)
 private:
  Expression* context_;
  Statement* body_;
};

class LabelledStatement : public Statement {
 public:
  LabelledStatement(Expression* expr, Statement* body)
    : body_(body) {
    label_ = expr->AsLiteral()->AsIdentifier();
  }
  inline Identifier* label() const { return label_; }
  inline Statement* body() const { return body_; }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(LabelledStatement)
 private:
  Identifier* label_;
  Statement* body_;
};

class CaseClause : public AstNode {
 public:
  explicit CaseClause(Space* factory)
    : expr_(NULL),
      body_(Statements::allocator_type(factory)),
      default_(false) {
  }

  void SetExpression(Expression* expr) {
    expr_ = expr;
  }

  void SetDefault() {
    default_ = true;
  }

  void AddStatement(Statement* stmt) {
    body_.push_back(stmt);
  }
  inline bool IsDefault() const {
    return default_;
  }
  inline Expression* expr() const {
    return expr_;
  }
  inline const Statements& body() const {
    return body_;
  }
  ACCEPT_VISITOR
 private:
  Expression* expr_;
  Statements body_;
  bool default_;
};

class SwitchStatement : public AnonymousBreakableStatement {
 public:
  typedef List<CaseClause*>::type CaseClauses;
  SwitchStatement(Expression* expr, Space* factory)
    : expr_(expr),
      clauses_(CaseClauses::allocator_type(factory)) {
  }

  void AddCaseClause(CaseClause* clause) {
    clauses_.push_back(clause);
  }
  inline Expression* expr() const { return expr_; }
  inline const CaseClauses& clauses() const { return clauses_; }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(SwitchStatement)
 private:
  Expression* expr_;
  CaseClauses clauses_;
};

class ThrowStatement : public Statement {
 public:
  explicit ThrowStatement(Expression* expr)
    : expr_(expr) {
  }
  inline Expression* expr() const { return expr_; }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(ThrowStatement)
 private:
  Expression* expr_;
};

class TryStatement : public Statement {
 public:
  explicit TryStatement(Block* block)
    : body_(block),
      catch_name_(NULL),
      catch_block_(NULL),
      finally_block_(NULL) {
  }

  void SetCatch(Identifier* name, Block* block) {
    catch_name_ = name;
    catch_block_ = block;
  }

  void SetFinally(Block* block) {
    finally_block_ = block;
  }
  inline Block* body() const { return body_; }
  inline Identifier* catch_name() const { return catch_name_; }
  inline Block* catch_block() const { return catch_block_; }
  inline Block* finally_block() const { return finally_block_; }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(TryStatement)
 private:
  Block* body_;
  Identifier* catch_name_;
  Block* catch_block_;
  Block* finally_block_;
};

class DebuggerStatement : public Statement {
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(DebuggerStatement)
};

class ExpressionStatement : public Statement {
 public:
  explicit ExpressionStatement(Expression* expr) : expr_(expr) { }
  inline Expression* expr() const { return expr_; }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(ExpressionStatement)
 private:
  Expression* expr_;
};

class Assignment : public Expression {
 public:
  Assignment(Token::Type op,
             Expression* left, Expression* right)
    : op_(op),
      left_(left),
      right_(right) {
  }
  inline Token::Type op() const { return op_; }
  inline Expression* left() const { return left_; }
  inline Expression* right() const { return right_; }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(Assignment)
 private:
  Token::Type op_;
  Expression* left_;
  Expression* right_;
};

class BinaryOperation : public Expression {
 public:
  BinaryOperation(Token::Type op,
                  Expression* left, Expression* right)
    : op_(op),
      left_(left),
      right_(right) {
  }
  inline Token::Type op() const { return op_; }
  inline Expression* left() const { return left_; }
  inline Expression* right() const { return right_; }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(BinaryOperation)
 private:
  Token::Type op_;
  Expression* left_;
  Expression* right_;
};

class ConditionalExpression : public Expression {
 public:
  ConditionalExpression(Expression* cond,
                        Expression* left,
                        Expression* right)
    : cond_(cond), left_(left), right_(right) {
  }
  inline Expression* cond() const { return cond_; }
  inline Expression* left() const { return left_; }
  inline Expression* right() const { return right_; }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(ConditionalExpression)
 private:
  Expression* cond_;
  Expression* left_;
  Expression* right_;
};

class UnaryOperation : public Expression {
 public:
  UnaryOperation(Token::Type op, Expression* expr)
    : op_(op),
      expr_(expr) {
  }
  inline Token::Type op() const { return op_; }
  inline Expression* expr() const { return expr_; }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(UnaryOperation)
 private:
  Token::Type op_;
  Expression* expr_;
};

class PostfixExpression : public Expression {
 public:
  PostfixExpression(Token::Type op, Expression* expr)
    : op_(op),
      expr_(expr) {
  }
  inline Token::Type op() const { return op_; }
  inline Expression* expr() const { return expr_; }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(PostfixExpression)
 private:
  Token::Type op_;
  Expression* expr_;
};

class StringLiteral : public Literal {
 public:
  StringLiteral(const std::vector<uc16>& buffer, Space* factory)
    : value_(buffer.data(), buffer.size(), SpaceUString::allocator_type(factory)) {
  }

  inline const SpaceUString& value() const {
    return value_;
  }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(StringLiteral)
 private:
  const SpaceUString value_;
};

class Directivable : public StringLiteral {
 public:
  explicit Directivable(const std::vector<uc16>& buffer, Space* factory)
    : StringLiteral(buffer, factory) { }
  DECLARE_NODE_TYPE(Directivable)
};

class NumberLiteral : public Literal {
 public:
  NumberLiteral(const double & val)
    : value_(val) {
  }
  inline double value() const { return value_; }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(NumberLiteral)
 private:
  double value_;
};

class Identifier : public Literal {
 public:
  typedef SpaceUString value_type;
  Identifier(const uc16* buffer, Space* factory)
    : value_(buffer, SpaceUString::allocator_type(factory)) {
  }
  Identifier(const char* buffer, Space* factory)
    : value_(buffer,
             buffer+std::char_traits<char>::length(buffer),
             SpaceUString::allocator_type(factory)) {
  }
  Identifier(const std::vector<uc16>& buffer, Space* factory)
    : value_(buffer.data(), buffer.size(), SpaceUString::allocator_type(factory)) {
  }
  Identifier(const std::vector<char>& buffer, Space* factory)
    : value_(buffer.begin(),
             buffer.end(), SpaceUString::allocator_type(factory)) {
  }
  inline const SpaceUString& value() const {
    return value_;
  }
  inline bool IsValidLeftHandSide() const { return true; }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(Identifier)
 protected:
  SpaceUString value_;
};

class IdentifierKey {
 public:
  typedef Identifier value_type;
  typedef IdentifierKey this_type;
  IdentifierKey(value_type* ident)  // NOLINT
    : ident_(ident) { }
  IdentifierKey(const IdentifierKey& rhs)
    : ident_(rhs.ident_) { }
  inline const value_type::value_type& value() const {
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

class ThisLiteral : public Literal {
 public:
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(ThisLiteral)
};

class NullLiteral : public Literal {
 public:
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(NullLiteral)
};

class TrueLiteral : public Literal {
 public:
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(TrueLiteral)
};

class FalseLiteral : public Literal {
 public:
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(FalseLiteral)
};

class Undefined : public Literal {
 public:
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(Undefined)
};

class RegExpLiteral : public Literal {
 public:
  RegExpLiteral(const std::vector<uc16>& buffer,
                const std::vector<uc16>& flags,
                Space* factory)
    : value_(buffer.data(), buffer.size(), SpaceUString::allocator_type(factory)),
      flags_(flags.data(), flags.size(), SpaceUString::allocator_type(factory)) {
  }
  inline const SpaceUString& value() const { return value_; }
  inline const SpaceUString& flags() const { return flags_; }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(RegExpLiteral)
 protected:
  SpaceUString value_;
  SpaceUString flags_;
};

class ArrayLiteral : public Literal {
 public:
  explicit ArrayLiteral(Space* factory)
    : items_(Expressions::allocator_type(factory)) {
  }
  inline void AddItem(Expression* expr) {
    items_.push_back(expr);
  }
  inline const Expressions& items() const {
    return items_;
  }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(ArrayLiteral)
 private:
  Expressions items_;
};

class ObjectLiteral : public Literal {
 public:
  enum PropertyDescriptorType {
    DATA = 1,
    GET  = 2,
    SET  = 4
  };
  typedef std::tr1::tuple<PropertyDescriptorType,
                          Identifier*,
                          Expression*> Property;
  typedef List<Property>::type Properties;
  explicit ObjectLiteral(Space* factory)
    : properties_(Properties::allocator_type(factory)) {
  }

  inline void AddDataProperty(Identifier* key, Expression* val) {
    AddPropertyDescriptor(DATA, key, val);
  }
  inline void AddAccessor(PropertyDescriptorType type,
                          Identifier* key, Expression* val) {
    AddPropertyDescriptor(type, key, val);
  }
  inline const Properties& properties() const {
    return properties_;
  }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(ObjectLiteral)
 private:
  inline void AddPropertyDescriptor(PropertyDescriptorType type,
                                    Identifier* key,
                                    Expression* val) {
    properties_.push_back(Properties::value_type(type, key, val));
  }
  Properties properties_;
};

class FunctionLiteral : public Literal {
 public:
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
  inline void SetName(Identifier* name) { name_ = name; }
  inline Identifier* name() const {
    return name_;
  }
  inline DeclType type() const { return type_; }
  inline const Identifiers& params() const {
    return params_;
  }
  inline const Statements& body() const {
    return body_;
  }
  inline Scope* scope() {
    return &scope_;
  }
  inline const Scope& scope() const {
    return scope_;
  }
  inline void set_start_position(std::size_t start) {
    start_position_ = start;
  }
  inline void set_end_position(std::size_t end) {
    end_position_ = end;
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
  inline void set_strict(bool strict) {
    strict_ = strict;
  }
  inline void SubStringSource(BasicSource* src) {
    source_ = src->SubString(start_position_,
                             end_position_ - start_position_ + 1);
  }
  inline UStringPiece GetSource() const {
    return source_;
  }
  FunctionLiteral(DeclType type, Space* factory)
    : name_(NULL),
      type_(type),
      params_(Identifiers::allocator_type(factory)),
      body_(Statements::allocator_type(factory)),
      scope_(factory),
      strict_(false) {
  }

  void AddParameter(Identifier* param) {
    params_.push_back(param);
  }

  void AddStatement(Statement* stmt) {
    body_.push_back(stmt);
  }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(FunctionLiteral)
 private:
  Identifier* name_;
  DeclType type_;
  Identifiers params_;
  Statements body_;
  Scope scope_;
  bool strict_;
  std::size_t start_position_;
  std::size_t end_position_;
  UStringPiece source_;
};

class PropertyAccess : public Expression {
 public:
  inline bool IsValidLeftHandSide() const { return true; }
  inline Expression* target() const { return target_; }
  DECLARE_NODE_TYPE(PropertyAccess)
 protected:
  explicit PropertyAccess(Expression* obj)
    : target_(obj) {
  }
  Expression* target_;
};

class IdentifierAccess : public PropertyAccess {
 public:
  IdentifierAccess(Expression* obj, Identifier* key)
    : PropertyAccess(obj),
      key_(key) {
  }
  inline Identifier* key() const { return key_; }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(IdentifierAccess)
 private:
  Identifier* key_;
};

class IndexAccess : public PropertyAccess {
 public:
  IndexAccess(Expression* obj, Expression* key)
    : PropertyAccess(obj),
      key_(key) {
  }
  inline Expression* key() const { return key_; }
  DECLARE_NODE_TYPE(IndexAccess)
  ACCEPT_VISITOR
 private:
  Expression* key_;
};

class Call : public Expression {
 public:
  inline bool IsValidLeftHandSide() const { return true; }
  Call(Expression* target, Space* factory)
    : target_(target),
      args_(Expressions::allocator_type(factory)) {
  }
  void AddArgument(Expression* expr) { args_.push_back(expr); }
  inline Expression* target() const { return target_; }
  inline const Expressions& args() const { return args_; }
  DECLARE_NODE_TYPE(Call)
 protected:
  Expression* target_;
  Expressions args_;
};

class FunctionCall : public Call {
 public:
  FunctionCall(Expression* target, Space* factory)
    : Call(target, factory) {
  }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(FunctionCall)
};

class ConstructorCall : public Call {
 public:
  ConstructorCall(Expression* target, Space* factory)
    : Call(target, factory) {
  }
  ACCEPT_VISITOR
  DECLARE_NODE_TYPE(ConstructorCall)
};

} }  // namespace iv::core

namespace std {
namespace tr1 {
// template specialization
// for iv::core::Parser::IdentifierWrapper in std::tr1::unordered_map
// allowed in section 17.4.3.1
template<>
struct hash<iv::core::IdentifierKey>
  : public unary_function<iv::core::IdentifierKey, std::size_t> {
  result_type operator()(const argument_type& x) const {
    return hash<argument_type::value_type::value_type>()(x.value());
  }
};

#undef ACCEPT_VISITOR
#undef DECLARE_NODE_TYPE
#undef DECLARE_NODE_TYPE_BASE

} }  // namespace std::tr1
#endif  // _IV_AST_H_
