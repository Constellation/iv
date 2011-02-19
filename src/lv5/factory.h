#ifndef _IV_LV5_FACTORY_H_
#define _IV_LV5_FACTORY_H_
#include <vector>
#include "symbol.h"
#include "alloc.h"
#include "ast_factory.h"
#include "location.h"
#include "ustringpiece.h"
#include "lv5/context_utils.h"
#include "lv5/jsast.h"

namespace iv {
namespace lv5 {
class Context;

class AstFactory
  : public core::Space<1>,
    public core::ast::BasicAstFactory<AstFactory> {
 public:
  typedef core::SpaceVector<AstFactory, RegExpLiteral*>::type DestReqs;
  explicit AstFactory(Context* ctx)
    : core::Space<1>(),
      core::ast::BasicAstFactory<AstFactory>(),
      ctx_(ctx),
      regexps_(DestReqs::allocator_type(this)) { }

  ~AstFactory() {
    for (DestReqs::const_iterator it = regexps_.begin(),
         last = regexps_.end(); it != last; ++it) {
      (*it)->~RegExpLiteral();
    }
  }

  template<typename Range>
  Identifier* NewIdentifier(core::Token::Type token,
                            const Range& range,
                            std::size_t begin, std::size_t end) {
    Identifier* ident = new (this) Identifier(range, this);
    ident->set_symbol(context::Intern(ctx_, ident->value()));
    return ident;
  }

  inline RegExpLiteral* NewRegExpLiteral(
      const std::vector<uc16>& content,
      const std::vector<uc16>& flags,
      std::size_t begin,
      std::size_t end) {
    RegExpLiteral* expr = new (this) RegExpLiteral(content, flags, this);
    expr->Initialize(ctx_);
    if (expr->IsValid()) {
      regexps_.push_back(expr);
      return expr;
    } else {
      return NULL;
    }
  }
 private:
  Context* ctx_;
  DestReqs regexps_;
};
} }  // namespace iv::lv5
#endif  // _IV_LV5_FACTORY_H_
