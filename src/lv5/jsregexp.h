#ifndef _IV_LV5_JSREGEXP_H_
#define _IV_LV5_JSREGEXP_H_
#include "jsobject.h"
#include "stringpiece.h"
#include "ustringpiece.h"
#include "jsregexp-impl.h"
namespace iv {
namespace lv5 {

class JSRegExp : public JSObject {
 public:
  JSRegExp(const core::UStringPiece& value,
           const core::UStringPiece& flags)
    : status_(U_ZERO_ERROR),
      impl_(new JSRegExpImpl(value, flags, &status_)) {
  }

  explicit JSRegExp(const JSRegExpImpl& reg)
    : status_(U_ZERO_ERROR),
      impl_(&reg) {
  }

  inline bool IsValid() const {
    return status_ == U_ZERO_ERROR;
  }

  bool IsCallable() const {
    return true;
  }

  static JSRegExp* New(const JSRegExpImpl& reg) {
    return new JSRegExp(reg);
  }

  static JSRegExp* New(const core::UStringPiece& value,
                       const core::UStringPiece& flags) {
    return new JSRegExp(value, flags);
  }

 private:
  UErrorCode status_;
  const JSRegExpImpl* impl_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSREGEXP_H_
