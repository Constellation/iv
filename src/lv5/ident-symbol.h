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
  template<typename Range>
  IdentifierWithSymbol(Context* ctx,
                       const Range& buffer,
                       AstFactory* factory)
    : Identifier(buffer, factory),
      sym_(Intern(ctx)) {
  }
  Symbol symbol() const {
    return sym_;
  }
 private:
  Symbol Intern(Context* ctx);
  Symbol sym_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_IDENT_SYMBOL_H_
