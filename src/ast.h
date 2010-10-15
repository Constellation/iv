#ifndef _IV_AST_H_
#define _IV_AST_H_
#include <vector>
#include <tr1/unordered_map>
#include <tr1/tuple>
#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/schriter.h>
#include "noncopyable.h"
#include "utils.h"
#include "alloc-inl.h"
#include "functor.h"
#include "token.h"
#include "scope.h"
#include "source.h"
#include "ast-visitor.h"

namespace iv {
namespace core {
#define ACCEPT_VISITOR \
  inline void Accept(AstVisitor* visitor) {\
    visitor->Visit(this);\
  }

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

class AstFactory;

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

  virtual Statement* AsStatement() { return NULL; }
  virtual ExpressionStatement* AsExpressionStatement() { return NULL; }
  virtual EmptyStatement* AsEmptyStatement() { return NULL; }
  virtual VariableStatement* AsVariableStatement() { return NULL; }
  virtual DebuggerStatement* AsDebuggerStatement() { return NULL; }
  virtual FunctionStatement* AsFunctionStatement() { return NULL; }
  virtual IfStatement* AsIfStatement() { return NULL; }
  virtual IterationStatement* AsIterationStatement() { return NULL; }
  virtual DoWhileStatement* AsDoWhileStatement() { return NULL; }
  virtual WhileStatement* AsWhileStatement() { return NULL; }
  virtual ForStatement* AsForStatement() { return NULL; }
  virtual ForInStatement* AsForInStatement() { return NULL; }
  virtual ContinueStatement* AsContinueStatement() { return NULL; }
  virtual BreakStatement* AsBreakStatement() { return NULL; }
  virtual ReturnStatement* AsReturnStatement() { return NULL; }
  virtual WithStatement* AsWithStatement() { return NULL; }
  virtual LabelledStatement* AsLabelledStatement() { return NULL; }
  virtual SwitchStatement* AsSwitchStatement() { return NULL; }
  virtual ThrowStatement* AsThrowStatement() { return NULL; }
  virtual TryStatement* AsTryStatement() { return NULL; }
  virtual BreakableStatement* AsBreakableStatement() { return NULL; }
  virtual NamedOnlyBreakableStatement* AsNamedOnlyBreakableStatement() {
    return NULL;
  }
  virtual AnonymousBreakableStatement* AsAnonymousBreakableStatement() {
    return NULL;
  }

  virtual Expression* AsExpression() { return NULL; }
  virtual Literal* AsLiteral() { return NULL; }
  virtual UnaryOperation* AsUnaryOperation() { return NULL; }
  virtual Assignment* AsAssignment() { return NULL; }
  virtual BinaryOperation* AsBinaryOperation() { return NULL; }
  virtual ConditionalExpression* AsConditionalExpression() { return NULL; }
  virtual PropertyAccess* AsPropertyAccess() { return NULL; }
  virtual IdentifierAccess* AsIdentifierAccess() { return NULL; }
  virtual IndexAccess* AsIndexAccess() { return NULL; }
  virtual Call* AsCall() { return NULL; }
  virtual FunctionCall* AsFunctionCall() { return NULL; }
  virtual ConstructorCall* AsConstructorCall() { return NULL; }

  virtual void Accept(AstVisitor* visitor) = 0;
};

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

class Statement : public AstNode {
 public:
  inline Statement* AsStatement() { return this; }
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
  BreakableStatement* AsBreakableStatement() { return this; }
 protected:
  Identifiers* labels_;
};

class NamedOnlyBreakableStatement : public BreakableStatement {
 public:
  inline NamedOnlyBreakableStatement* AsNamedOnlyBreakableStatement() {
    return this;
  }
};

class AnonymousBreakableStatement : public BreakableStatement {
 public:
  inline AnonymousBreakableStatement* AsAnonymousBreakableStatement() {
    return this;
  }
};

class Block : public NamedOnlyBreakableStatement {
 public:
  explicit Block(Space *factory);
  void AddStatement(Statement *stmt);
  inline Block* AsBlock() { return this; }
  ACCEPT_VISITOR
  inline const Statements& body() {
    return body_;
  }
 private:
  Statements body_;
};

class FunctionStatement : public Statement {
 public:
  explicit FunctionStatement(FunctionLiteral* func);
  inline FunctionStatement* AsFunctionStatement() { return this; }
  inline FunctionLiteral* function() {
    return function_;
  }
  ACCEPT_VISITOR
 private:
  FunctionLiteral* function_;
};

class VariableStatement : public Statement {
 public:
  explicit VariableStatement(Token::Type type, Space* factory);
  void AddDeclaration(Declaration* decl);
  inline VariableStatement* AsVariableStatement() { return this; }
  inline const Declarations& decls() {
    return decls_;
  }
  inline bool IsConst() const {
    return is_const_;
  }
  ACCEPT_VISITOR
 private:
  const bool is_const_;
  Declarations decls_;
};

class Declaration : public AstNode {
 public:
  Declaration(Identifier* name, Expression* expr);
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
  inline EmptyStatement* AsEmptyStatement() { return this; }
  static inline EmptyStatement* New(Space* f) {
    return new (f) EmptyStatement();
  }
  ACCEPT_VISITOR
};

class IfStatement : public Statement {
 public:
  IfStatement(Expression* cond, Statement* then);
  void SetElse(Statement* stmt);
  inline IfStatement* AsIfStatement() { return this; }
  inline Expression* cond() { return cond_; }
  inline Statement* then_statement() { return then_; }
  inline Statement* else_statement() { return else_; }
  ACCEPT_VISITOR
 private:
  Expression* cond_;
  Statement* then_;
  Statement* else_;
};

class IterationStatement : public AnonymousBreakableStatement {
 public:
  IterationStatement();
  inline IterationStatement* AsIterationStatement() { return this; }
  inline Statement* body() { return body_; }
  inline void set_body(Statement* stmt) {
    body_ = stmt;
  }
 private:
  Statement* body_;
};

class DoWhileStatement : public IterationStatement {
 public:
  DoWhileStatement();
  inline DoWhileStatement* AsDoWhileStatement() { return this; }
  inline Expression* cond() { return cond_; }
  inline void set_cond(Expression* expr) {
    cond_ = expr;
  }
  ACCEPT_VISITOR
 private:
  Expression* cond_;
};

class WhileStatement : public IterationStatement {
 public:
  explicit WhileStatement(Expression* cond);
  inline WhileStatement* AsWhileStatement() { return this; }
  inline Expression* cond() { return cond_; }
  ACCEPT_VISITOR
 private:
  Expression* cond_;
};

class ForStatement : public IterationStatement {
 public:
  ForStatement();
  inline void SetInit(Statement* init) { init_ = init; }
  inline void SetCondition(Expression* cond) { cond_ = cond; }
  inline void SetNext(Statement* next) { next_ = next; }
  inline ForStatement* AsForStatement() { return this; }
  inline Statement* init() { return init_; }
  inline Expression* cond() { return cond_; }
  inline Statement* next() { return next_; }
  ACCEPT_VISITOR
 private:
  Statement* init_;
  Expression* cond_;
  Statement* next_;
};

class ForInStatement : public IterationStatement {
 public:
  ForInStatement(Statement* each, Expression* enumerable);
  inline ForInStatement* AsForInStatement() { return this; }
  inline Statement* each() { return each_; }
  inline Expression* enumerable() { return enumerable_; }
  ACCEPT_VISITOR
 private:
  Statement* each_;
  Expression* enumerable_;
};

class ContinueStatement : public Statement {
 public:
  ContinueStatement();
  void SetLabel(Identifier* label);
  void SetTarget(IterationStatement* target);
  inline ContinueStatement* AsContinueStatement() { return this; }
  inline Identifier* label() { return label_; }
  inline IterationStatement* target() { return target_; }
  ACCEPT_VISITOR
 private:
  Identifier* label_;
  IterationStatement* target_;
};

class BreakStatement : public Statement {
 public:
  BreakStatement();
  void SetLabel(Identifier* label);
  void SetTarget(BreakableStatement* target);
  inline BreakStatement* AsBreakStatement() { return this; }
  inline Identifier* label() { return label_; }
  inline BreakableStatement* target() { return target_; }
  ACCEPT_VISITOR
 private:
  Identifier* label_;
  BreakableStatement* target_;
};

class ReturnStatement : public Statement {
 public:
  explicit ReturnStatement(Expression* expr);
  inline ReturnStatement* AsReturnStatement() { return this; }
  inline Expression* expr() { return expr_; }
  ACCEPT_VISITOR
 private:
  Expression* expr_;
};

class WithStatement : public Statement {
 public:
  WithStatement(Expression* context, Statement* body);
  inline WithStatement* AsWithStatement() { return this; }
  inline Expression* context() { return context_; }
  inline Statement* body() { return body_; }
  ACCEPT_VISITOR
 private:
  Expression* context_;
  Statement* body_;
};

class LabelledStatement : public Statement {
 public:
  explicit LabelledStatement(Expression* expr, Statement* body);
  inline LabelledStatement* AsLabelledStatement() { return this; }
  inline Identifier* label() { return label_; }
  inline Statement* body() { return body_; }
  ACCEPT_VISITOR
 private:
  Identifier* label_;
  Statement* body_;
};

class CaseClause : public AstNode {
 public:
  explicit CaseClause(Space* factory);
  void SetExpression(Expression* expr);
  void SetDefault();
  void AddStatement(Statement* stmt);
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
  explicit SwitchStatement(Expression* expr, Space* factory);
  void AddCaseClause(CaseClause* clause);
  inline SwitchStatement* AsSwitchStatement() { return this; }
  inline Expression* expr() { return expr_; }
  inline const CaseClauses& clauses() { return clauses_; }
  ACCEPT_VISITOR
 private:
  Expression* expr_;
  CaseClauses clauses_;
};

class ThrowStatement : public Statement {
 public:
  explicit ThrowStatement(Expression* expr);
  inline ThrowStatement* AsThrowStatement() { return this; }
  inline Expression* expr() { return expr_; }
  ACCEPT_VISITOR
 private:
  Expression* expr_;
};

class TryStatement : public Statement {
 public:
  explicit TryStatement(Block* block);
  void SetCatch(Identifier* name, Block* block);
  void SetFinally(Block* block);
  inline TryStatement* AsTryStatement() { return this; }
  inline Block* body() { return body_; }
  inline Identifier* catch_name() { return catch_name_; }
  inline Block* catch_block() { return catch_block_; }
  inline Block* finally_block() { return finally_block_; }
  ACCEPT_VISITOR
 private:
  Block* body_;
  Identifier* catch_name_;
  Block* catch_block_;
  Block* finally_block_;
};

class DebuggerStatement : public Statement {
  inline DebuggerStatement* AsDebuggerStatement() { return this; }
  ACCEPT_VISITOR
};

class ExpressionStatement : public Statement {
 public:
  explicit ExpressionStatement(Expression* expr);
  inline ExpressionStatement* AsExpressionStatement() { return this; }
  inline Expression* expr() { return expr_; }
  ACCEPT_VISITOR
 private:
  Expression* expr_;
};

// Expression
class Expression : public AstNode {
 public:
  inline virtual bool IsValidLeftHandSide() const { return false; }
  inline Expression* AsExpression() { return this; }
};

class Assignment : public Expression {
 public:
  Assignment(Token::Type op, Expression* left, Expression* right);
  inline Assignment* AsAssignment() { return this; }
  inline Token::Type op() { return op_; }
  inline Expression* left() { return left_; }
  inline Expression* right() { return right_; }
  ACCEPT_VISITOR
 private:
  Token::Type op_;
  Expression* left_;
  Expression* right_;
};

class BinaryOperation : public Expression {
 public:
  BinaryOperation(Token::Type op, Expression* left, Expression* right);
  inline BinaryOperation* AsBinaryOperation() { return this; }
  inline Token::Type op() { return op_; }
  inline Expression* left() { return left_; }
  inline Expression* right() { return right_; }
  ACCEPT_VISITOR
 private:
  Token::Type op_;
  Expression* left_;
  Expression* right_;
};

class ConditionalExpression : public Expression {
 public:
  ConditionalExpression(Expression* cond, Expression* left, Expression* right);
  inline ConditionalExpression* AsConditionalExpression() { return this; }
  inline Expression* cond() { return cond_; }
  inline Expression* left() { return left_; }
  inline Expression* right() { return right_; }
  ACCEPT_VISITOR
 private:
  Expression* cond_;
  Expression* left_;
  Expression* right_;
};

class UnaryOperation : public Expression {
 public:
  UnaryOperation(Token::Type op, Expression* expr);
  inline UnaryOperation* AsUnaryOperation() { return this; }
  inline Token::Type op() { return op_; }
  inline Expression* expr() { return expr_; }
  ACCEPT_VISITOR
 private:
  Token::Type op_;
  Expression* expr_;
};

class PostfixExpression : public Expression {
 public:
  PostfixExpression(Token::Type op, Expression* expr);
  inline PostfixExpression* AsPostfixExpression() { return this; }
  inline Token::Type op() { return op_; }
  inline Expression* expr() { return expr_; }
  ACCEPT_VISITOR
 private:
  Token::Type op_;
  Expression* expr_;
};

class ThisLiteral;
class NullLiteral;
class TrueLiteral;
class FalseLiteral;
class NumberLiteral;
class StringLiteral;
class RegExpLiteral;
class ArrayLiteral;
class ObjectLiteral;
class FunctionLiteral;
class Undefined;

class Literal : public Expression {
 public:
  Literal* AsLiteral() { return this; }
  virtual ~Literal() = 0;
  virtual ThisLiteral* AsThisLiteral() { return NULL; }
  virtual NullLiteral* AsNullLiteral() { return NULL; }
  virtual FalseLiteral* AsFalseLiteral() { return NULL; }
  virtual TrueLiteral* AsTrueLiteral() { return NULL; }
  virtual Undefined* AsUndefined() { return NULL; }
  virtual NumberLiteral* AsNumberLiteral() { return NULL; }
  virtual StringLiteral* AsStringLiteral() { return NULL; }
  virtual Identifier* AsIdentifier() { return NULL; }
  virtual RegExpLiteral* AsRegExpLiteral() { return NULL; }
  virtual ArrayLiteral* AsArrayLiteral() { return NULL; }
  virtual ObjectLiteral* AsObjectLiteral() { return NULL; }
  virtual FunctionLiteral* AsFunctionLiteral() { return NULL; }
};

class StringLiteral : public Literal {
 public:
  explicit StringLiteral(const std::vector<UChar>& buffer, Space* factory);
  inline StringLiteral* AsStringLiteral() { return this; }
  inline const SpaceUString& value() {
    return value_;
  }
  ACCEPT_VISITOR
 private:
  const SpaceUString value_;
};

class NumberLiteral : public Literal {
 public:
  explicit NumberLiteral(const double & val);
  inline NumberLiteral* AsNumberLiteral() { return this; }
  inline double value() const { return value_; }
  ACCEPT_VISITOR
  static NumberLiteral* New(Space* f, const double & val) {
    return new (f) NumberLiteral(val);
  }
 private:
  double value_;
};

class Identifier : public Literal {
 public:
  typedef SpaceUString value_type;
  explicit Identifier(const UChar* buffer, Space* factory);
  explicit Identifier(const char* buffer, Space* factory);
  explicit Identifier(const std::vector<UChar>& buffer, Space* factory);
  explicit Identifier(const std::vector<char>& buffer, Space* factory);
  inline Identifier* AsIdentifier() { return this; }
  inline const SpaceUString& value() const {
    return value_;
  }
  inline bool IsValidLeftHandSide() const { return true; }
  ACCEPT_VISITOR
 private:
  SpaceUString value_;
};

class IdentifierKey {
 public:
  typedef Identifier value_type;
  typedef IdentifierKey this_type;
  IdentifierKey(value_type* ident)  // NOLINT
    : ident_(ident) { }
  IdentifierKey(const this_type& rhs)
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
  inline ThisLiteral* AsThisLiteral() { return this; }
  ACCEPT_VISITOR
  static ThisLiteral* New(Space* f) {
    return new (f) ThisLiteral();
  }
};

class NullLiteral : public Literal {
 public:
  inline NullLiteral* AsNullLiteral() { return this; }
  ACCEPT_VISITOR
  static NullLiteral* New(Space* f) {
    return new (f) NullLiteral();
  }
};

class TrueLiteral : public Literal {
 public:
  inline TrueLiteral* AsTrueLiteral() { return this; }
  ACCEPT_VISITOR
  static TrueLiteral* New(Space* f) {
    return new (f) TrueLiteral();
  }
};

class FalseLiteral : public Literal {
 public:
  inline FalseLiteral* AsFalseLiteral() { return this; }
  ACCEPT_VISITOR
  static FalseLiteral* New(Space* f) {
    return new (f) FalseLiteral();
  }
};

class Undefined : public Literal {
 public:
  inline Undefined* AsUndefined() { return this; }
  ACCEPT_VISITOR
  static inline Undefined* New(Space* f) {
    return new (f) Undefined();
  }
};

class RegExpLiteral : public Literal {
 public:
  explicit RegExpLiteral(const std::vector<UChar>& buffer, Space* factory);
  inline RegExpLiteral* AsRegExpLiteral() { return this; }
  void SetFlags(const std::vector<UChar>& buffer);
  inline const SpaceUString& value() { return value_; }
  inline const SpaceUString& flags() { return flags_; }
  ACCEPT_VISITOR
 private:
  SpaceUString value_;
  SpaceUString flags_;
};

class ArrayLiteral : public Literal {
 public:
  explicit ArrayLiteral(Space* factory);
  inline ArrayLiteral* AsArrayLiteral() { return this; }
  inline void AddItem(Expression* expr) {
    items_.push_back(expr);
  }
  inline const Expressions& items() {
    return items_;
  }
  ACCEPT_VISITOR
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
  explicit ObjectLiteral(Space* factory);

  inline void AddDataProperty(Identifier* key, Expression* val) {
    AddPropertyDescriptor(DATA, key, val);
  }
  inline void AddAccessor(PropertyDescriptorType type,
                          Identifier* key, Expression* val) {
    AddPropertyDescriptor(type, key, val);
  }

  inline ObjectLiteral* AsObjectLiteral() { return this; }
  inline
  const Properties& properties() {
    return properties_;
  }
  ACCEPT_VISITOR
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
  FunctionLiteral(DeclType type, Space* factory);
  inline FunctionLiteral* AsFunctionLiteral() { return this; }
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
  inline void set_source(Source* src) {
    source_ = src;
  }
  inline UStringPiece GetSource() const {
    return source_->SubString(start_position_, end_position_ - start_position_ + 1);
  }
  void AddParameter(Identifier* param);
  void AddStatement(Statement* stmt);
  ACCEPT_VISITOR
 private:
  Identifier* name_;
  DeclType type_;
  Identifiers params_;
  Statements body_;
  Scope scope_;
  bool strict_;
  std::size_t start_position_;
  std::size_t end_position_;
  Source* source_;
};

class PropertyAccess : public Expression {
 public:
  inline bool IsValidLeftHandSide() const { return true; }
  inline Expression* target() { return target_; }
  inline PropertyAccess* AsPropertyAccess() { return this; }
 protected:
  explicit PropertyAccess(Expression* obj);
  Expression* target_;
};

class IdentifierAccess : public PropertyAccess {
 public:
  IdentifierAccess(Expression* obj, Identifier* key)
    : PropertyAccess(obj),
      key_(key) {
  }
  inline Identifier* key() { return key_; }
  inline IdentifierAccess* AsIdentifierAccess() { return this; }
  ACCEPT_VISITOR
 private:
  Identifier* key_;
};

class IndexAccess : public PropertyAccess {
 public:
  IndexAccess(Expression* obj, Expression* key)
    : PropertyAccess(obj),
      key_(key) {
  }
  inline Expression* key() { return key_; }
  inline IndexAccess* AsIndexAccess() { return this; }
  ACCEPT_VISITOR
 private:
  Expression* key_;
};

class Call : public Expression {
 public:
  inline bool IsValidLeftHandSide() const { return true; }
  explicit Call(Expression* target, Space* factory);
  void AddArgument(Expression* expr) { args_.push_back(expr); }
  inline Call* AsCall() { return this; }
  inline Expression* target() { return target_; }
  inline const Expressions& args() { return args_; }
 protected:
  Expression* target_;
  Expressions args_;
};

class FunctionCall : public Call {
 public:
  explicit FunctionCall(Expression* target, Space* factory);
  inline FunctionCall* AsFunctionCall() { return this; }
  ACCEPT_VISITOR
};

class ConstructorCall : public Call {
 public:
  explicit ConstructorCall(Expression* target, Space* factory);
  inline ConstructorCall* AsConstructorCall() { return this; }
  ACCEPT_VISITOR
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

} }  // namespace std::tr1
#endif  // _IV_AST_H_
