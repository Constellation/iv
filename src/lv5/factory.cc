#include "jsast.h"
#include "context.h"
#include "factory.h"

namespace iv {
namespace lv5 {
Symbol AstFactory::Intern(const Identifier& ident) {
  return ctx_->Intern(ident.value());
}
} }  // namespace iv::lv5
