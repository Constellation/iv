#include "ident-symbol.h"
#include "context.h"

namespace iv {
namespace lv5 {

Symbol IdentifierWithSymbol::Intern(Context* ctx) {
  return ctx->Intern(value());
}


} }  // namespace iv::lv5
