#include <algorithm>
#include "ast.h"

namespace iv {
namespace core {

AstNode::~AstNode() { }

Block::Block(Space* factory) : body_(Statements::allocator_type(factory)) { }

void Block::AddStatement(Statement* stmt) {
  body_.push_back(stmt);
}

FunctionStatement::FunctionStatement(FunctionLiteral* func)
  : function_(func) {
}

VariableStatement::VariableStatement(Token::Type type, Space* factory)
  : is_const_(type == Token::CONST),
    decls_(Declarations::allocator_type(factory)) { }

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

IterationStatement::IterationStatement()
  : body_(NULL) {
}

DoWhileStatement::DoWhileStatement()
  : IterationStatement(),
    cond_(NULL) {
}

WhileStatement::WhileStatement(Expression* cond)
  : IterationStatement(),
    cond_(cond) {
}

ForStatement::ForStatement()
  : IterationStatement(),
    init_(NULL),
    cond_(NULL),
    next_(NULL) {
}

ForInStatement::ForInStatement(Statement* each,
                               Expression* enumerable)
  : IterationStatement(),
    each_(each),
    enumerable_(enumerable) {
}

ContinueStatement::ContinueStatement()
  : label_(NULL) {
}

void ContinueStatement::SetLabel(Identifier* label) {
  label_ = label;
}

void ContinueStatement::SetTarget(IterationStatement* target) {
  target_ = target;
}

BreakStatement::BreakStatement()
  : label_(NULL),
    target_(NULL) {
}

void BreakStatement::SetLabel(Identifier* label) {
  label_ = label;
}

void BreakStatement::SetTarget(BreakableStatement* target) {
  target_ = target;
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
    clauses_(CaseClauses::allocator_type(factory)) {
}

void SwitchStatement::AddCaseClause(CaseClause* clause) {
  clauses_.push_back(clause);
}

CaseClause::CaseClause(Space* factory)
  : expr_(NULL),
    body_(Statements::allocator_type(factory)),
    default_(false) {
}

void CaseClause::SetExpression(Expression* expr) {
  expr_ = expr;
}

void CaseClause::SetDefault() {
  default_ = true;
}

void CaseClause::AddStatement(Statement* stmt) {
  body_.push_back(stmt);
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

StringLiteral::StringLiteral(const std::vector<UChar>& buffer, Space* factory)
  : value_(buffer.data(), buffer.size(), SpaceUString::allocator_type(factory)) {
}

NumberLiteral::NumberLiteral(const double & val)
  : value_(val) {
}

Identifier::Identifier(const UChar* buffer, Space* factory)
  : value_(buffer, SpaceUString::allocator_type(factory)) {
}

Identifier::Identifier(const char* buffer, Space* factory)
  : value_(buffer,
           buffer+std::char_traits<char>::length(buffer),
           SpaceUString::allocator_type(factory)) {
}

Identifier::Identifier(const std::vector<UChar>& buffer, Space* factory)
  : value_(buffer.data(), buffer.size(), SpaceUString::allocator_type(factory)) {
}

Identifier::Identifier(const std::vector<char>& buffer, Space* factory)
  : value_(buffer.begin(),
           buffer.end(), SpaceUString::allocator_type(factory)) {
}

RegExpLiteral::RegExpLiteral(const std::vector<UChar>& buffer,
                             const std::vector<UChar>& flags,
                             Space* factory)
  : value_(buffer.data(), buffer.size(), SpaceUString::allocator_type(factory)),
    flags_(flags.data(), flags.size(), SpaceUString::allocator_type(factory)) {
}

ArrayLiteral::ArrayLiteral(Space* factory)
  : items_(Expressions::allocator_type(factory)) {
}

ObjectLiteral::ObjectLiteral(Space* factory)
  : properties_(Properties::allocator_type(factory)) {
}

FunctionLiteral::FunctionLiteral(DeclType type, Space* factory)
  : name_(NULL),
    type_(type),
    params_(Identifiers::allocator_type(factory)),
    body_(Statements::allocator_type(factory)),
    scope_(factory),
    strict_(false) {
}

void FunctionLiteral::AddParameter(Identifier* param) {
  params_.push_back(param);
}

void FunctionLiteral::AddStatement(Statement* stmt) {
  body_.push_back(stmt);
}

PropertyAccess::PropertyAccess(Expression* obj)
  : target_(obj) {
}

Call::Call(Expression* target, Space* factory)
  : target_(target),
    args_(Expressions::allocator_type(factory)) {
}

ConstructorCall::ConstructorCall(Expression* target, Space* factory)
  : Call(target, factory) {
}

FunctionCall::FunctionCall(Expression* target, Space* factory)
  : Call(target, factory) {
}

} }  // namespace iv::core
