#include "regexp-icu.h"
#include "factory.h"

namespace iv {
namespace lv5 {

ICURegExpLiteral::ICURegExpLiteral(
    const std::vector<uc16>& buffer,
    const std::vector<uc16>& flags,
    AstFactory* space)
  : RegExpLiteral(buffer, flags, space),
    status_(U_ZERO_ERROR),
    regexp_(value_, flags_, &status_) {
}

RegExpLiteral* RegExpICU::Create(AstFactory* space,
                                 const std::vector<uc16>& content,
                                 const std::vector<uc16>& flags) {
  ICURegExpLiteral* expr = new (space) ICURegExpLiteral(content, flags, space);
  if (expr->IsValid()) {
    return expr;
  } else {
    return NULL;
  }
}

} }  // namespace iv::lv5
