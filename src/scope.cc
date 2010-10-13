#include "scope.h"
#include "ast.h"

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

bool Scope::Contains(const Identifier& ident) const {
  const SpaceUString& target = ident.value();
  for (Variables::const_iterator it = vars_.begin(),
       last = vars_.end(); it != last; ++it) {
    if (it->first->value() == target) {
      return true;
    }
  }
  for (FunctionLiterals::const_iterator it = funcs_.begin(),
       last = funcs_.end(); it != last; ++it) {
    if ((*it)->name()->value() == target) {
      return true;
    }
  }
  return false;
}

} }  // namespace iv::core
