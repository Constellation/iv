#ifndef _IV_LV5_REGEXP_ICU_H_
#define _IV_LV5_REGEXP_ICU_H_
#include <vector>
#include "uchar.h"
#include "ast.h"
#include "jsregexp.h"
namespace iv {

namespace core {
class Space;
}  // namespace iv::core

namespace lv5 {
class ICURegExpLiteral : public core::RegExpLiteral {
 public:
  ICURegExpLiteral(const std::vector<uc16>& buffer,
                   const std::vector<uc16>& flags,
                   core::Space* space)
    : RegExpLiteral(buffer, flags, space),
      status_(U_ZERO_ERROR),
      regexp_(value_, flags_, &status_) {
  }
  inline const JSRegExpImpl& regexp() const {
    return regexp_;
  }

  inline bool IsValid() const {
    return status_ == U_ZERO_ERROR;
  }

 private:
  UErrorCode status_;
  JSRegExpImpl regexp_;
};

class RegExpICU {
 public:
  static core::RegExpLiteral* Create(core::Space* space,
                                     const std::vector<uc16>& content,
                                     const std::vector<uc16>& flags) {
    ICURegExpLiteral* expr = new(space)ICURegExpLiteral(content, flags, space);
    if (expr->IsValid()) {
      return expr;
    } else {
      return NULL;
    }
  }
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_REGEXP_ICU_H_
