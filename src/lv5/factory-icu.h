#ifndef _IV_LV5_FACTORY_ICU_H_
#define _IV_LV5_FACTORY_ICU_H_
#include <vector>
#include "regexp-icu.h"
#include "ast-factory.h"
namespace iv {
namespace core {
template<>
class AstFactory<lv5::RegExpICU> : public BasicAstFactory {
 public:
  typedef AstFactory<lv5::RegExpICU> this_type;
  ~AstFactory<lv5::RegExpICU>() {
    for (std::vector<RegExpLiteral*>::const_iterator it = regexps_.begin(),
         last = regexps_.end(); it != last; ++it) {
      (*it)->~RegExpLiteral();
    }
  }
  RegExpLiteral* NewRegExpLiteral(const std::vector<UChar>& content,
                                  const std::vector<UChar>& flags) {
    RegExpLiteral* reg = lv5::RegExpICU::Create(this, content, flags);
    if (reg) {
      regexps_.push_back(reg);
    }
    return reg;
  }
 private:
  std::vector<RegExpLiteral*> regexps_;
};
} }  // namespace iv::lv5
#endif  // _IV_LV5_FACTORY_ICU_H_
