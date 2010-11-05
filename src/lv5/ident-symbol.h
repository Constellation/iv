#ifndef _IV_LV5_IDENT_SYMBOL_H_
#define _IV_LV5_IDENT_SYMBOL_H_
#include "ast.h"
#include "jsast.h"
#include "symbol.h"
#include "ustringpiece.h"

namespace iv {
namespace lv5 {
class Context;
class AstFactory;

class IdentifierWithSymbol : public Identifier {
 public:
  IdentifierWithSymbol(Context* ctx,
                       const core::UStringPiece& buffer,
                       AstFactory* factory);
  IdentifierWithSymbol(Context* ctx,
                       const std::vector<uc16>& buffer,
                       AstFactory* factory);
  IdentifierWithSymbol(Context* ctx,
                       const std::vector<char>& buffer,
                       AstFactory* factory);
  Symbol symbol() const {
    return sym_;
  }
 private:
  Symbol sym_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_IDENT_SYMBOL_H_
