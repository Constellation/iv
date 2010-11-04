#ifndef _IV_LV5_IDENT_SYMBOL_H_
#define _IV_LV5_IDENT_SYMBOL_H_
#include "ast.h"
#include "symbol.h"
#include "context.h"
#include "ustringpiece.h"
namespace iv {
namespace core {
class Space;
}  // namespace iv::core
namespace lv5 {

class IdentifierWithSymbol : public core::Identifier {
 public:
  IdentifierWithSymbol(Context* ctx,
                       const core::UStringPiece& buffer, core::Space* factory)
    : Identifier(buffer, factory),
      sym_(ctx->Intern(value_)) {
  }
  IdentifierWithSymbol(Context* ctx,
                       const std::vector<uc16>& buffer, core::Space* factory)
    : Identifier(buffer, factory),
      sym_(ctx->Intern(value_)) {
  }
  IdentifierWithSymbol(Context* ctx,
                       const std::vector<char>& buffer, core::Space* factory)
    : Identifier(buffer, factory),
      sym_(ctx->Intern(value_)) {
  }
  Symbol symbol() const {
    return sym_;
  }
 private:
  Symbol sym_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_IDENT_SYMBOL_H_
