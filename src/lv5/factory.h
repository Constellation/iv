#ifndef _IV_LV5_FACTORY_H_
#define _IV_LV5_FACTORY_H_
#include <vector>
#include "alloc.h"
#include "regexp-icu.h"
#include "ident-symbol.h"
#include "ast-factory.h"
#include "location.h"
#include "ustringpiece.h"

namespace iv {
namespace lv5 {
class Context;

class AstFactory : public core::ast::BasicAstFactory<2, AstFactory> {
 public:
  typedef core::SpaceVector<AstFactory, RegExpLiteral*>::type DestReqs;
  explicit AstFactory(Context* ctx)
    : ctx_(ctx),
      regexps_(DestReqs::allocator_type(this)) { }

  ~AstFactory() {
    for (DestReqs::const_iterator it = regexps_.begin(),
         last = regexps_.end(); it != last; ++it) {
      (*it)->~RegExpLiteral();
    }
  }

  template<typename Range>
  Identifier* NewIdentifier(const Range& range) {
    return new (this) IdentifierWithSymbol(ctx_, range, this);
  }

  inline RegExpLiteral* NewRegExpLiteral(
      const std::vector<uc16>& content,
      const std::vector<uc16>& flags) {
    RegExpLiteral* reg = RegExpICU::Create(this, content, flags);
    if (reg) {
      regexps_.push_back(reg);
    }
    return reg;
  }
 private:
  Context* ctx_;
  DestReqs regexps_;
};
} }  // namespace iv::lv5
#endif  // _IV_LV5_FACTORY_H_
