#include "ast-factory.h"
namespace iv {
namespace core {

AstFactory::AstFactory()
  : Space(),
    undefined_instance_(),
    empty_statement_instance_(),
    debugger_statement_instance_(),
    this_instance_(),
    null_instance_(),
    registered_() {
}

AstFactory::~AstFactory() {
  std::for_each(registered_.begin(), registered_.end(), Destructor<AstNode>());
}

void AstFactory::Register(AstNode* node) {
  registered_.push_back(node);
}

Identifier* AstFactory::NewIdentifier(const UChar* buffer) {
  Identifier* ident = new(this) Identifier(buffer);
  Register(ident);
  return ident;
}

Identifier* AstFactory::NewIdentifier(const char* buffer) {
  Identifier* ident = new(this) Identifier(buffer);
  Register(ident);
  return ident;
}

StringLiteral* AstFactory::NewStringLiteral(const UChar* buffer) {
  StringLiteral* str = new(this) StringLiteral(buffer);
  Register(str);
  return str;
}

RegExpLiteral* AstFactory::NewRegExpLiteral(const UChar* buffer) {
  RegExpLiteral* reg = new(this) RegExpLiteral(buffer);
  Register(reg);
  return reg;
}

FunctionLiteral* AstFactory::NewFunctionLiteral(FunctionLiteral::Type type) {
  FunctionLiteral* func = new(this) FunctionLiteral(type);
  Register(func);
  return func;
}

ArrayLiteral* AstFactory::NewArrayLiteral() {
  ArrayLiteral* array = new(this) ArrayLiteral(this);
  Register(array);
  return array;
}

ObjectLiteral* AstFactory::NewObjectLiteral() {
  ObjectLiteral* obj = new(this) ObjectLiteral();
  Register(obj);
  return obj;
}

} }  // namespace iv::core

