#include "ast-factory.h"
namespace iv {
namespace core {

AstFactory::AstFactory()
  : Space(),
    undefined_instance_(new (this) Undefined()),
    empty_statement_instance_(new (this) EmptyStatement()),
    debugger_statement_instance_(new (this) DebuggerStatement()),
    this_instance_(new (this) ThisLiteral()),
    null_instance_(new (this) NullLiteral()),
    true_instance_(new (this) TrueLiteral()),
    false_instance_(new (this) FalseLiteral()) {
}

Identifier* AstFactory::NewIdentifier(const UChar* buffer) {
  return new (this) Identifier(buffer, this);
}

Identifier* AstFactory::NewIdentifier(const char* buffer) {
  return new (this) Identifier(buffer, this);
}

Identifier* AstFactory::NewIdentifier(const std::vector<UChar>& buffer) {
  return new (this) Identifier(buffer, this);
}

Identifier* AstFactory::NewIdentifier(const std::vector<char>& buffer) {
  return new (this) Identifier(buffer, this);
}

StringLiteral* AstFactory::NewStringLiteral(const std::vector<UChar>& buffer) {
  return new (this) StringLiteral(buffer, this);
}

RegExpLiteral* AstFactory::NewRegExpLiteral(const std::vector<UChar>& buffer) {
  return new (this) RegExpLiteral(buffer, this);
}

FunctionLiteral* AstFactory::NewFunctionLiteral(
    FunctionLiteral::DeclType type) {
  return new (this) FunctionLiteral(type, this);
}

ArrayLiteral* AstFactory::NewArrayLiteral() {
  return new (this) ArrayLiteral(this);
}

ObjectLiteral* AstFactory::NewObjectLiteral() {
  return new (this) ObjectLiteral(this);
}

AstNode::Identifiers* AstFactory::NewLabels() {
  typedef AstNode::Identifiers Identifiers;
  return new (New(sizeof(Identifiers)))
      Identifiers(Identifiers::allocator_type(this));
}

} }  // namespace iv::core
