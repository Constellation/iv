#ifndef _IV_LV5_FACTORY_H_
#define _IV_LV5_FACTORY_H_
#include <vector>
#include "alloc-inl.h"
#include "regexp-icu.h"
#include "ident-symbol.h"
#include "ast-factory.h"
#include "context.h"
namespace iv {
namespace lv5 {
class AstFactory : public core::BasicAstFactory {
 public:
  typedef core::AstNode::List<core::RegExpLiteral*>::type DestReqs;
  explicit AstFactory(Context* ctx)
    : ctx_(ctx),
      regexps_(DestReqs::allocator_type(this)) { }

  ~AstFactory() {
    for (DestReqs::const_iterator it = regexps_.begin(),
         last = regexps_.end(); it != last; ++it) {
      (*it)->~RegExpLiteral();
    }
  }

  core::Identifier* NewIdentifier(const uc16* buffer) {
    return new (this) IdentifierWithSymbol(ctx_, buffer, this);
  }

  core::Identifier* NewIdentifier(const char* buffer) {
    return new (this) IdentifierWithSymbol(ctx_, buffer, this);
  }

  core::Identifier* NewIdentifier(const std::vector<uc16>& buffer) {
    return new (this) IdentifierWithSymbol(ctx_, buffer, this);
  }

  core::Identifier* NewIdentifier(const std::vector<char>& buffer) {
    return new (this) IdentifierWithSymbol(ctx_, buffer, this);
  }

  inline core::RegExpLiteral* NewRegExpLiteral(
      const std::vector<uc16>& content,
      const std::vector<uc16>& flags) {
    core::RegExpLiteral* reg = RegExpICU::Create(this, content, flags);
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
