#ifndef _IV_LV5_REGEXP_ICU_H_
#define _IV_LV5_REGEXP_ICU_H_
#include <vector>
#include "uchar.h"
#include "jsregexp.h"
#include "jsast.h"

namespace iv {
namespace lv5 {

class ICURegExpLiteral : public RegExpLiteral {
 public:
  ICURegExpLiteral(const std::vector<uc16>& buffer,
                   const std::vector<uc16>& flags,
                   AstFactory* space);
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
  static RegExpLiteral* Create(AstFactory* space,
                               const std::vector<uc16>& content,
                               const std::vector<uc16>& flags);
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_REGEXP_ICU_H_
