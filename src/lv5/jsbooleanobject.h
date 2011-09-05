#ifndef IV_LV5_JSBOOLEANOBJECT_H_
#define IV_LV5_JSBOOLEANOBJECT_H_
#include "lv5/jsobject.h"
#include "lv5/map.h"
#include "lv5/context_utils.h"
namespace iv {
namespace lv5 {

class JSBooleanObject : public JSObject {
 public:
  JSBooleanObject(Context* ctx, bool value)
    : JSObject(context::GetBooleanMap(ctx)),
      value_(value) {
  }

  JSBooleanObject(Context* ctx, Map* map, bool value)
    : JSObject(map),
      value_(value) {
  }

  bool value() const {
    return value_;
  }

  static const Class* GetClass() {
    static const Class cls = {
      "Boolean",
      Class::Boolean
    };
    return &cls;
  }

  static JSBooleanObject* New(Context* ctx, bool value) {
    JSBooleanObject* const obj = new JSBooleanObject(ctx, value);
    obj->set_cls(JSBooleanObject::GetClass());
    obj->set_prototype(context::GetClassSlot(ctx, Class::Boolean).prototype);
    return obj;
  }

  static JSBooleanObject* NewPlain(Context* ctx, Map* map, bool value) {
    return new JSBooleanObject(ctx, map, value);
  }

 private:
  bool value_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSBOOLEANOBJECT_H_
