#ifndef _IV_LV5_FACTORY_ICU_H_
#define _IV_LV5_FACTORY_ICU_H_
#include <vector>
#include "alloc-inl.h"
#include "regexp-icu.h"
#include "ast-factory.h"
namespace iv {
namespace lv5 {

class Lv5AstFactory : public core::BasicAstFactory {
 public:
  typedef core::AstNode::List<core::RegExpLiteral*>::type DestReqs;
  Lv5AstFactory()
    : regexps_(DestReqs::allocator_type(this)) { }

  ~Lv5AstFactory() {
    for (DestReqs::const_iterator it = regexps_.begin(),
         last = regexps_.end(); it != last; ++it) {
      (*it)->~RegExpLiteral();
    }
  }

  inline core::RegExpLiteral* NewRegExpLiteral(
      const std::vector<UChar>& content,
      const std::vector<UChar>& flags) {
    core::RegExpLiteral* reg = RegExpICU::Create(this, content, flags);
    if (reg) {
      regexps_.push_back(reg);
    }
    return reg;
  }
 private:
  DestReqs regexps_;
};
} }  // namespace iv::lv5
#endif  // _IV_LV5_FACTORY_ICU_H_
