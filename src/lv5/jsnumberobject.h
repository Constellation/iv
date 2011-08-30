#ifndef IV_LV5_JSNUMBEROBJECT_H_
#define IV_LV5_JSNUMBEROBJECT_H_
#include "lv5/jsobject.h"
#include "lv5/context_utils.h"
namespace iv {
namespace lv5 {

class JSNumberObject : public JSObject {
 public:
  explicit JSNumberObject(const double& value) : value_(value) { }
  const double& value() const {
    return value_;
  }

  static const Class* GetClass() {
    static const Class cls = {
      "Number",
      Class::Number
    };
    return &cls;
  }

  static JSNumberObject* New(Context* ctx, const double& value) {
    JSNumberObject* const obj = new JSNumberObject(value);
    obj->set_cls(JSNumberObject::GetClass());
    obj->set_prototype(context::GetClassSlot(ctx, Class::Number).prototype);
    return obj;
  }

  static JSNumberObject* NewPlain(Context* ctx, const double& value) {
    return new JSNumberObject(value);
  }

 private:
  double value_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSNUMBEROBJECT_H_
