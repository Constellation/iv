#ifndef IV_LV5_REFERENCE_H_
#define IV_LV5_REFERENCE_H_
#include <gc/gc_cpp.h>
#include <iv/lv5/jsval_fwd.h>
#include <iv/lv5/symbol.h>
#include <iv/lv5/radio/cell.h>
#include <iv/lv5/radio/core_fwd.h>
namespace iv {
namespace lv5 {

class JSReference : public radio::HeapObject<radio::REFERENCE> {
 public:
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

  void MarkChildren(radio::Core* core) {
    core->MarkValue(base_);
  }

 private:
  JSReference(JSVal base, Symbol name, bool is_strict)
    : base_(base),
      name_(name),
      is_strict_(is_strict) {
  }

  JSVal base_;
  Symbol name_;
  bool is_strict_;
};
} }  // namespace iv::lv5
#endif  // IV_LV5_JSREFERENCE_H_
