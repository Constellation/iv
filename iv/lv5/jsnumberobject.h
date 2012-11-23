#ifndef IV_LV5_JSNUMBEROBJECT_H_
#define IV_LV5_JSNUMBEROBJECT_H_
#include <iv/lv5/jsobject.h>
#include <iv/lv5/map.h>
namespace iv {
namespace lv5 {

class JSNumberObject : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(Number)

  double value() const {
    return value_;
  }

  static JSNumberObject* New(Context* ctx, double value) {
    JSNumberObject* const obj = new JSNumberObject(ctx, value);
    obj->set_cls(JSNumberObject::GetClass());
    return obj;
  }

  static JSNumberObject* NewPlain(Context* ctx, Map* map, double value) {
    return new JSNumberObject(ctx, map, value);
  }

 private:
  JSNumberObject(Context* ctx, double value)
    : JSObject(ctx->global_data()->number_map()),
      value_(value) {
  }

  JSNumberObject(Context* ctx, Map* map, double value)
    : JSObject(map),
      value_(value) {
  }

  double value_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSNUMBEROBJECT_H_
