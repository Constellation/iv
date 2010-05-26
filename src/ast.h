#ifndef _IV_AST_H_
#define _IV_AST_H_
#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>
#include <cstdio>
#include <vector>
#include <sstream>
#include <map>
#include <algorithm>
#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/schriter.h>
#include "utils.h"
#include "alloc-inl.h"
#include "functor.h"
#include "token.h"
#include "ast-visitor.h"

namespace iv {
namespace core {
#define VIRTUAL_VISITOR \
  virtual void Accept(AstVisitor* visitor) = 0;

#define ACCEPT_VISITOR \
  inline void Accept(AstVisitor* visitor) {\
    visitor->Visit(this);\
  }\

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

class Expression;
class Literal;
class FunctionLiteral;
class Assignment;
class UnaryOperation;
class BinaryOperation;
class ConditionalExpression;
class PropertyAccess;
class Call;
class ConstructorCall;
class FunctionCall;
class Identifier;
class Declaration;

class AstFactory;

class AstNode : public SpaceObject {
 public:
  explicit AstNode();
  virtual ~AstNode() { }

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

  virtual Expression* AsExpression() { return NULL; }
  virtual Literal* AsLiteral() { return NULL; }
  virtual UnaryOperation* AsUnaryOperation() { return NULL; }
  virtual Assignment* AsAssignment() { return NULL; }
  virtual BinaryOperation* AsBinaryOperation() { return NULL; }
  virtual ConditionalExpression* AsConditionalExpression() { return NULL; }
  virtual PropertyAccess* AsPropertyAccess() { return NULL; }
  virtual Call* AsCall() { return NULL; }
  virtual FunctionCall* AsFunctionCall() { return NULL; }
  virtual ConstructorCall* AsConstructorCall() { return NULL; }

  virtual llvm::Value* Codegen() { return NULL; }

  VIRTUAL_VISITOR
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

class Block : public Statement {
 public:
  explicit Block(Space *factory);
  void AddStatement(Statement *stmt);
  inline Block* AsBlock() { return this; }
  ACCEPT_VISITOR
  inline const SpaceVector<Statement*>::type& body() {
    return body_;
  }
 private:
  SpaceVector<Statement*>::type body_;
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
  inline const SpaceVector<Declaration*>::type& decls() {
    return decls_;
  }
  inline bool IsConst() const {
    return is_const_;
  }
  ACCEPT_VISITOR
 private:
  const bool is_const_;
  SpaceVector<Declaration*>::type decls_;
};

class Declaration : public AstNode {
 public:
  Declaration(Identifier* name, Expression* expr);
  inline Identifier* name() {
    return name_;
  }
  inline Expression* expr() {
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
  llvm::Value* Codegen();
 private:
  Expression* cond_;
  Statement* then_;
  Statement* else_;
};

class IterationStatement : public Statement {
 public:
  friend class AstVisitor;
  explicit IterationStatement(Statement* body);
  inline IterationStatement* AsIterationStatement() { return this; }
  inline Statement* body() { return body_; }
 private:
  Statement* body_;
};

class DoWhileStatement : public IterationStatement {
 public:
  DoWhileStatement(Statement* body, Expression* cond);
  inline DoWhileStatement* AsDoWhileStatement() { return this; }
  inline Expression* cond() { return cond_; }
  ACCEPT_VISITOR
 private:
  Expression* cond_;
};

class WhileStatement : public IterationStatement {
 public:
  WhileStatement(Statement* body, Expression* cond);
  inline WhileStatement* AsWhileStatement() { return this; }
  inline Expression* cond() { return cond_; }
  ACCEPT_VISITOR
 private:
  Expression* cond_;
};

class ForStatement : public IterationStatement {
 public:
  explicit ForStatement(Statement* body);
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
  ForInStatement(Statement* each, Expression* enumerable, Statement* body);
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
  inline ContinueStatement* AsContinueStatement() { return this; }
  inline Identifier* label() { return label_; }
  ACCEPT_VISITOR
 private:
  Identifier* label_;
};

class BreakStatement : public Statement {
 public:
  BreakStatement();
  void SetLabel(Identifier* label);
  inline BreakStatement* AsBreakStatement() { return this; }
  inline Identifier* label() { return label_; }
  ACCEPT_VISITOR
 private:
  Identifier* label_;
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
  CaseClause();
  void SetExpression(Expression* expr);
  void SetDefault();
  void SetStatement(Statement* stmt);
  inline bool IsDefault() const {
    return default_;
  }
  inline Expression* expr() {
    return expr_;
  }
  inline Statement* body() {
    return body_;
  }
  ACCEPT_VISITOR
 private:
  Expression* expr_;
  Statement* body_;
  bool default_;
};

class SwitchStatement : public Statement {
 public:
  explicit SwitchStatement(Expression* expr, Space* factory);
  void AddCaseClause(CaseClause* clause);
  inline SwitchStatement* AsSwitchStatement() { return this; }
  inline Expression* expr() { return expr_; }
  inline const SpaceVector<CaseClause*>::type& clauses() { return clauses_; }
  ACCEPT_VISITOR
 private:
  Expression* expr_;
  SpaceVector<CaseClause*>::type clauses_;
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
  explicit StringLiteral(const UChar* buffer);
  inline StringLiteral* AsStringLiteral() { return this; }
  inline const UnicodeString& value() {
    return value_;
  }
  static StringLiteral* New(Space* f, const UChar* buffer) {
    return new (f) StringLiteral(buffer);
  }
  ACCEPT_VISITOR
 private:
  UnicodeString value_;
};

class NumberLiteral : public Literal {
 public:
  explicit NumberLiteral(const double & val);
  inline NumberLiteral* AsNumberLiteral() { return this; }
  inline double value() const { return value_; }
  ACCEPT_VISITOR
  llvm::Value* Codegen();
  static NumberLiteral* New(Space* f, const double & val) {
    return new (f) NumberLiteral(val);
  }
 private:
  double value_;
};

class Identifier : public Literal {
 public:
  explicit Identifier(const UChar* buffer);
  explicit Identifier(const char* buffer);
  inline Identifier* AsIdentifier() { return this; }
  inline const UnicodeString& value() const {
    return value_;
  }
  inline bool IsValidLeftHandSide() const { return true; }
  ACCEPT_VISITOR
 private:
  UnicodeString value_;
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
  llvm::Value* Codegen() {
    return llvm::ConstantInt::get(
        llvm::Type::getInt1Ty(llvm::getGlobalContext()), 1);
  }
  static TrueLiteral* New(Space* f) {
    return new (f) TrueLiteral();
  }
};

class FalseLiteral : public Literal {
 public:
  inline FalseLiteral* AsFalseLiteral() { return this; }
  ACCEPT_VISITOR
  llvm::Value* Codegen() {
    return llvm::ConstantInt::get(
        llvm::Type::getInt1Ty(llvm::getGlobalContext()), 0);
  }
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
  explicit RegExpLiteral(const UChar* buffer);
  inline RegExpLiteral* AsRegExpLiteral() { return this; }
  void SetFlags(const UChar* flags);
  inline const UnicodeString& value() { return value_; }
  inline const UnicodeString& flags() { return flags_; }
  ACCEPT_VISITOR
  static RegExpLiteral* New(Space* f, const UChar* buffer) {
    return new (f) RegExpLiteral(buffer);
  }
 private:
  UnicodeString value_;
  UnicodeString flags_;
};

class ArrayLiteral : public Literal {
 public:
  explicit ArrayLiteral(Space* factory);
  inline ArrayLiteral* AsArrayLiteral() { return this; }
  inline void AddItem(Expression* expr) {
    items_.push_back(expr);
  }
  inline const SpaceVector<Expression*>::type& items() {
    return items_;
  }
  ACCEPT_VISITOR
 private:
  SpaceVector<Expression*>::type items_;
};

class ObjectLiteral : public Literal {
 public:
  ObjectLiteral();
  void AddProperty(Identifier* key, Expression* val);
  inline ObjectLiteral* AsObjectLiteral() { return this; }
  inline const std::map<Identifier*, Expression*>& properties() {
    return properties_;
  }
  ACCEPT_VISITOR
 private:
  std::map<Identifier*, Expression*> properties_;
};

class FunctionLiteral : public Literal {
 public:
  enum Type {
    DECLARATION,
    STATEMENT,
    EXPRESSION,
    GLOBAL
  };
  explicit FunctionLiteral(Type type);
  inline FunctionLiteral* AsFunctionLiteral() { return this; }
  inline void SetName(const UChar* name) { name_ = name; }
  inline const UnicodeString& name() {
    return name_;
  }
  inline Type  type() { return type_; }
  inline const std::vector<Identifier*>& params() {
    return params_;
  }
  inline const std::vector<Statement*>& body() {
    return body_;
  }
  void AddParameter(Identifier* param);
  void AddStatement(Statement* stmt);
  ACCEPT_VISITOR
 private:
  UnicodeString name_;
  Type type_;
  std::vector<Identifier*> params_;
  std::vector<Statement*> body_;
};

class PropertyAccess : public Expression {
 public:
  PropertyAccess(Expression* obj, Expression* key);
  inline bool IsValidLeftHandSide() const { return true; }
  inline Expression* target() { return target_; }
  inline Expression* key() { return key_; }
  inline PropertyAccess* AsPropertyAccess() { return this; }
  ACCEPT_VISITOR
 private:
  Expression* target_;
  Expression* key_;
};

class Call : public Expression {
 public:
  explicit Call(Expression* target, Space* factory);
  void AddArgument(Expression* expr) { args_.push_back(expr); }
  inline Call* AsCall() { return this; }
  inline Expression* target() { return target_; }
  inline const SpaceVector<Expression*>::type& args() { return args_; }
 protected:
  Expression* target_;
  SpaceVector<Expression*>::type args_;
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

class Scope : public AstNode {
 public:
  Scope() : up_(NULL) { }
  ~Scope() { }
  void NewParameter() { }
  void NewUnresolved() { }
 private:
  const Scope* up_;
};

} }  // namespace iv::core
#endif  // _IV_AST_H_

