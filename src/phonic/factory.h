#ifndef _IV_PHONIC_FACTORY_H_
#define _IV_PHONIC_FACTORY_H_
#include <vector>
#include "alloc-inl.h"
#include "regexp-icu.h"
#include "ast-factory.h"
namespace iv {
namespace phonic {

class AstFactory : public core::BasicAstFactory {
 public:
  typedef core::AstNode::List<core::RegExpLiteral*>::type DestReqs;
  AstFactory()
    : regexps_(DestReqs::allocator_type(this)) { }

  ~AstFactory() {
    for (DestReqs::const_iterator it = regexps_.begin(),
         last = regexps_.end(); it != last; ++it) {
      (*it)->~RegExpLiteral();
    }
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
  DestReqs regexps_;
};
} }  // namespace iv::phonic
#endif  // _IV_PHONIC_FACTORY_H_
