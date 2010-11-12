#ifndef _IV_LV5_JSREGEXP_IMPL_H_
#define _IV_LV5_JSREGEXP_IMPL_H_
#include <unicode/regex.h>
#include <gc/gc_cpp.h>
#include "ustringpiece.h"

namespace iv {
namespace lv5 {
class JSRegExpImpl : public gc_cleanup {
 public:
  JSRegExpImpl(const core::UStringPiece& value,
               const core::UStringPiece& flags,
               UErrorCode* status);
  JSRegExpImpl();
  void Initialize(const core::UStringPiece& value,
                  const core::UStringPiece& flags,
                  UErrorCode* status);
  ~JSRegExpImpl();
 private:
  URegularExpression* regexp_;
  bool global_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSREGEXP_IMPL_H_
