#ifndef IV_LV5_JSBOOLEANOBJECT_H_
#define IV_LV5_JSBOOLEANOBJECT_H_
#include <iv/lv5/jsobject.h>
#include <iv/lv5/map.h>
#include <iv/lv5/context_utils.h>
namespace iv {
namespace lv5 {

class JSBooleanObject : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(Boolean)

  bool value() const {
    return value_;
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
  JSBooleanObject(Context* ctx, bool value)
    : JSObject(context::GetBooleanMap(ctx)),
      value_(value) {
  }

  JSBooleanObject(Context* ctx, Map* map, bool value)
    : JSObject(map),
      value_(value) {
  }

  bool value_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSBOOLEANOBJECT_H_
