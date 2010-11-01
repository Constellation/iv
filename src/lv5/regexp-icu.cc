#include "regexp-icu.h"
#include "alloc-inl.h"

namespace iv {
namespace lv5 {

ICURegExpLiteral::ICURegExpLiteral(const std::vector<UChar>& buffer,
                                   const std::vector<UChar>& flags,
                                   core::Space* space)
  : RegExpLiteral(buffer, flags, space),
    status_(U_ZERO_ERROR),
    regexp_(value_, flags_, &status_) {
}

core::RegExpLiteral* RegExpICU::Create(core::Space* space,
                                       const std::vector<UChar>& content,
                                       const std::vector<UChar>& flags) {
  ICURegExpLiteral* expr = new(space)ICURegExpLiteral(content, flags, space);
  if (expr->IsValid()) {
    return expr;
  } else {
    return NULL;
  }
}

} }  // namespace iv::lv5
