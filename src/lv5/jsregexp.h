#ifndef _IV_LV5_JSREGEXP_H_
#define _IV_LV5_JSREGEXP_H_
#include "jsobject.h"
#include "stringpiece.h"
#include "ustringpiece.h"
#include "jsregexp_impl.h"
namespace iv {
namespace lv5 {

class JSRegExp : public JSObject {
 public:
  JSRegExp(const core::UStringPiece& value,
           const core::UStringPiece& flags)
    : impl_(new JSRegExpImpl(value, flags)) {
  }

  explicit JSRegExp(const JSRegExpImpl& reg)
    : impl_(&reg) {
  }

  inline bool IsValid() const {
    return impl_->IsValid();
  }

  static JSRegExp* New(const JSRegExpImpl& reg) {
    return new JSRegExp(reg);
  }

  static JSRegExp* New(const core::UStringPiece& value,
                       const core::UStringPiece& flags) {
    return new JSRegExp(value, flags);
  }

 private:
  const JSRegExpImpl* impl_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSREGEXP_H_
