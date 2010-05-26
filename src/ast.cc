#include "ast.h"

namespace {

llvm::IRBuilder<> builder(llvm::getGlobalContext());

}

namespace iv {
namespace core {

AstNode::AstNode() { }

Block::Block(Space* factory) : body_(SpaceAllocator<Statement*>(factory)) { }

void Block::AddStatement(Statement* stmt) {
  body_.push_back(stmt);
}

FunctionStatement::FunctionStatement(FunctionLiteral* func)
  : function_(func) {
}

VariableStatement::VariableStatement(Token::Type type, Space* factory)
  : is_const_(type == Token::CONST),
    decls_(SpaceAllocator<Declaration*>(factory)) { }

void VariableStatement::AddDeclaration(Declaration* decl) {
  decls_.push_back(decl);
}

Declaration::Declaration(Identifier* name, Expression* expr)
  : name_(name),
    expr_(expr) {
}

IfStatement::IfStatement(Expression* cond, Statement* then)
  : cond_(cond),
    then_(then),
    else_(NULL) {
}

void IfStatement::SetElse(Statement* stmt) {
  else_ = stmt;
}

llvm::Value* IfStatement::Codegen() {
  return NULL;
}

IterationStatement::IterationStatement(Statement* body)
  : body_(body) {
}

DoWhileStatement::DoWhileStatement(Statement* body, Expression* cond)
  : IterationStatement(body),
    cond_(cond) {
}

WhileStatement::WhileStatement(Statement* body, Expression* cond)
  : IterationStatement(body),
    cond_(cond) {
}

ForStatement::ForStatement(Statement* body)
  : IterationStatement(body),
    init_(NULL),
    cond_(NULL),
    next_(NULL) {
}

ForInStatement::ForInStatement(Statement* each,
                               Expression* enumerable, Statement* body)
  : IterationStatement(body),
    each_(each),
    enumerable_(enumerable) {
}

ContinueStatement::ContinueStatement()
  : label_(NULL) {
}

void ContinueStatement::SetLabel(Identifier* label) {
  label_ = label;
}

BreakStatement::BreakStatement()
  : label_(NULL) {
}

void BreakStatement::SetLabel(Identifier* label) {
  label_ = label;
}

ReturnStatement::ReturnStatement(Expression* expr)
  : expr_(expr) {
}

WithStatement::WithStatement(Expression* context, Statement* body)
  : context_(context),
    body_(body) {
}

LabelledStatement::LabelledStatement(Expression* expr, Statement* body)
  : body_(body) {
  label_ = expr->AsLiteral()->AsIdentifier();
}

SwitchStatement::SwitchStatement(Expression* expr, Space* factory)
  : expr_(expr),
    clauses_(SpaceAllocator<CaseClause*>(factory)) {
}

void SwitchStatement::AddCaseClause(CaseClause* clause) {
  clauses_.push_back(clause);
}

CaseClause::CaseClause()
  : expr_(NULL),
    body_(NULL),
    default_(false) {
}

void CaseClause::SetExpression(Expression* expr) {
  expr_ = expr;
}

void CaseClause::SetDefault() {
  default_ = true;
}

void CaseClause::SetStatement(Statement* stmt) {
  body_ = stmt;
}

ThrowStatement::ThrowStatement(Expression* expr)
  : expr_(expr) {
}

TryStatement::TryStatement(Block* block)
  : body_(block),
    catch_name_(NULL),
    catch_block_(NULL),
    finally_block_(NULL) {
}

void TryStatement::SetCatch(Identifier* name, Block* block) {
  catch_name_ = name;
  catch_block_ = block;
}

void TryStatement::SetFinally(Block* block) {
  finally_block_ = block;
}

ExpressionStatement::ExpressionStatement(Expression* expr) : expr_(expr) { }

Assignment::Assignment(Token::Type op,
                       Expression* left, Expression* right)
  : op_(op),
    left_(left),
    right_(right) {
}

BinaryOperation::BinaryOperation(Token::Type op,
                                 Expression* left, Expression* right)
  : op_(op),
    left_(left),
    right_(right) {
}

ConditionalExpression::ConditionalExpression(Expression* cond,
                                             Expression* left,
                                             Expression* right)
  : cond_(cond), left_(left), right_(right) {
}

UnaryOperation::UnaryOperation(Token::Type op, Expression* expr)
  : op_(op),
    expr_(expr) {
}

PostfixExpression::PostfixExpression(Token::Type op, Expression* expr)
  : op_(op),
    expr_(expr) {
}

Literal::~Literal() { }

StringLiteral::StringLiteral(const UChar* buffer)
  : value_(buffer) {
}

NumberLiteral::NumberLiteral(const double & val)
  : value_(val) {
}

llvm::Value* NumberLiteral::Codegen() {
  return llvm::ConstantFP::get(
      llvm::Type::getDoubleTy(llvm::getGlobalContext()), value_);
}

Identifier::Identifier(const UChar* buffer)
  : value_(buffer) {
}

Identifier::Identifier(const char* buffer)
  : value_(buffer) {
}

RegExpLiteral::RegExpLiteral(const UChar* buffer)
  : value_(buffer),
    flags_() {
}

void RegExpLiteral::SetFlags(const UChar* flags) {
  flags_ = flags;
}

ArrayLiteral::ArrayLiteral(Space* factory)
  : items_(SpaceAllocator<Expression*>(factory)) {
}

ObjectLiteral::ObjectLiteral() : properties_() {
}

void ObjectLiteral::AddProperty(Identifier* key, Expression* val) {
  properties_.insert(std::map<Identifier*, Expression*>::value_type(key, val));
}

FunctionLiteral::FunctionLiteral(Type type)
  : name_(""),
    type_(type),
    params_(),
    body_() {
}

void FunctionLiteral::AddParameter(Identifier* param) {
  params_.push_back(param);
}

void FunctionLiteral::AddStatement(Statement* stmt) {
  body_.push_back(stmt);
}

PropertyAccess::PropertyAccess(Expression* obj, Expression* key)
  : target_(obj),
    key_(key) {
}

Call::Call(Expression* target, Space* factory)
  : target_(target),
    args_(SpaceAllocator<Expression*>(factory)) {
}

ConstructorCall::ConstructorCall(Expression* target, Space* factory)
  : Call(target, factory) {
}

FunctionCall::FunctionCall(Expression* target, Space* factory)
  : Call(target, factory) {
}

} }  // namespace iv::core

