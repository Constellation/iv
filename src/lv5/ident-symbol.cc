#include "ident-symbol.h"
#include "context.h"

namespace iv {
namespace lv5 {

IdentifierWithSymbol::IdentifierWithSymbol(
    Context* ctx,
    const core::UStringPiece& buffer,
    AstFactory* factory)
  : Identifier(buffer, factory),
    sym_(ctx->Intern(value_)) {
}

IdentifierWithSymbol::IdentifierWithSymbol(
    Context* ctx,
    const std::vector<uc16>& buffer,
    AstFactory* factory)
  : Identifier(buffer, factory),
    sym_(ctx->Intern(value_)) {
}

IdentifierWithSymbol::IdentifierWithSymbol(
    Context* ctx,
    const std::vector<char>& buffer,
    AstFactory* factory)
  : Identifier(buffer, factory),
    sym_(ctx->Intern(value_)) {
}

} }  // namespace iv::lv5
