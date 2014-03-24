#ifndef IV_LV5_JSSTRINGOBJECT_H_
#define IV_LV5_JSSTRINGOBJECT_H_
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/jsstring.h>
#include <iv/lv5/map.h>
#include <iv/lv5/slot.h>
namespace iv {
namespace lv5 {

class JSStringObject : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(JSStringObject, String)

  IV_LV5_INTERNAL_METHOD bool GetOwnNonIndexedPropertySlotMethod(const JSObject* obj,
                                                                 Context* ctx,
                                                                 Symbol name, Slot* slot) {
    JSString* value = static_cast<const JSStringObject*>(obj)->value();
    if (name == symbol::length()) {
      slot->set(
          JSVal::Int32(static_cast<int32_t>(value->size())),
          Attributes::String::Length(), obj);
      return true;
    }
    return JSObject::GetOwnNonIndexedPropertySlotMethod(obj, ctx, name, slot);
  }

  IV_LV5_INTERNAL_METHOD bool GetOwnIndexedPropertySlotMethod(const JSObject* obj,
                                                              Context* ctx,
                                                              uint32_t index, Slot* slot) {
    JSString* value = static_cast<const JSStringObject*>(obj)->value();
    const std::size_t len = value->size();
    if (index < len) {
      slot->set(JSString::New(ctx, value->At(index)), Attributes::String::Indexed(), obj);
      return true;
    }
    return JSObject::GetOwnIndexedPropertySlotMethod(obj, ctx, index, slot);
  }

  IV_LV5_INTERNAL_METHOD void GetOwnPropertyNamesMethod(const JSObject* obj,
                                                        Context* ctx,
                                                        PropertyNamesCollector* collector,
                                                        EnumerationMode mode) {
    if (mode == INCLUDE_NOT_ENUMERABLE) {
      collector->Add(symbol::length(), 0);
    }
    JSString* value = static_cast<const JSStringObject*>(obj)->value();
    for (uint32_t i = 0, len = value->size(); i < len; ++i) {
      collector->Add(i);
    }
    JSObject::GetOwnPropertyNamesMethod(obj, ctx, collector, mode);
  }

  JSString* value() const {
    return value_;
  }

  static JSStringObject* New(Context* ctx, JSString* str) {
    JSStringObject* const obj = new JSStringObject(ctx, ctx->global_data()->string_map(), str);
    obj->set_cls(GetClass());
    return obj;
  }

  static JSStringObject* NewPlain(Context* ctx, Map* map) {
    return new JSStringObject(ctx, map, JSString::NewEmpty(ctx));
  }

  virtual void MarkChildren(radio::Core* core) {
    core->MarkCell(value_);
  }

 private:
  JSStringObject(Context* ctx, Map* map, JSString* value)
    : JSObject(map),
      value_(value) {
  }

  JSString* value_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSSTRINGOBJECT_H_
