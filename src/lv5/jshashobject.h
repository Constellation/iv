// default JS object, like
// var obj = { };
// is this `JSHashObject`
//
// JSObject is base all Objects, and not have default hash table,
// but, JSHashObject has hash table by default.
#ifndef IV_LV5_JSHASHOBJECT_H_
#define IV_LV5_JSHASHOBJECT_H_
#include "lv5/jsobject.h"
namespace iv {
namespace lv5 {

class JSHashObject : public JSObject {
 public:
  static JSHashObject* New(Context* ctx) {
    JSHashObject* const obj = NewPlain(ctx);
    obj->set_cls(JSHashObject::GetClass());
    obj->set_prototype(context::GetClassSlot(ctx, Class::Object).prototype);
    return obj;
  }

  static JSHashObject* NewPlain(Context* ctx) {
    return new JSHashObject();
  }

  static const Class* GetClass() {
    static const Class cls = {
      "Object",
      Class::Object
    };
    return &cls;
  }

 private:
  JSHashObject()
    : JSObject(&content_),
      content_() {
  }

  Properties content_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSHASHOBJECT_H_
