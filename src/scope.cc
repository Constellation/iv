#include "scope.h"

namespace iv {
namespace core {

void Scope::AddFunctionDeclaration(FunctionLiteral* func) {
  funcs_.push_back(func);
}

void Scope::SetUpperScope(Scope* scope) {
  up_ = scope;
}
Scope* Scope::GetUpperScope() {
  return up_;
}

} }  // namespace iv::core
