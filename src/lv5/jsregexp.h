#ifndef _IV_LV5_JSREGEXP_H_
#define _IV__JSREGEXP_H_
#include <unicode/regex.h>
#include <gc/gc_cpp.h>
#include "jsobject.h"
#include "alloc.h"
#include "stringpiece.h"
#include "ustringpiece.h"
namespace iv {
namespace lv5 {

class JSRegExp : public JSObject {
 public:
  JSRegExp(core::UStringPiece value,
           core::UStringPiece flags);

  inline bool IsValid() const {
    return status_ == U_ZERO_ERROR;
  }

  bool IsCallable() const {
    return true;
  }

  static JSRegExp* New(core::UStringPiece value,
                       core::UStringPiece flags);

 private:
  class JSRegExpImpl : public gc_cleanup {
   public:
    JSRegExpImpl(const core::UStringPiece& value,
                 uint32_t flags, bool is_global, UErrorCode* status);
    ~JSRegExpImpl();
   private:
    URegularExpression* regexp_;
    bool global_;
  };
  UErrorCode status_;
  JSRegExpImpl* impl_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSREGEXP_H_
