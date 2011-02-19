#ifndef _IV_LV5_REFERENCE_H_
#define _IV_LV5_REFERENCE_H_
#include <gc/gc_cpp.h>
#include "lv5/jsval.h"
#include "lv5/symbol.h"
namespace iv {
namespace lv5 {

class JSReference : public gc {
 public:
  JSReference(JSVal base, Symbol name, bool is_strict)
    : base_(base),
      name_(name),
      is_strict_(is_strict) {
  }
  bool IsStrictReference() const {
    return is_strict_;
  }
  JSVal GetBase() const {
    return base_;
  }
  Symbol GetReferencedName() const {
    return name_;
  }
  bool IsUnresolvableReference() const {
    return base_.IsUndefined();
  }
  bool HasPrimitiveBase() const {
    return base_.IsPrimitive();
  }
  bool IsPropertyReference() const {
    return base_.IsObject() || HasPrimitiveBase();
  }
  inline const JSVal& base() const {
    return base_;
  }
  static JSReference* New(Context* ctx,
                          JSVal base, Symbol name, bool is_strict) {
    return new JSReference(base, name, is_strict);
  }

 private:
  JSVal base_;
  Symbol name_;
  bool is_strict_;
};
} }  // namespace iv::lv5
#endif  // _IV_LV5_JSREFERENCE_H_
