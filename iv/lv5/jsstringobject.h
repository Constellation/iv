#ifndef IV_LV5_JSSTRINGOBJECT_H_
#define IV_LV5_JSSTRINGOBJECT_H_
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsstring.h>
#include <iv/lv5/map.h>
#include <iv/lv5/slot.h>
#include <iv/lv5/context_utils.h>
namespace iv {
namespace lv5 {

class JSStringObject : public JSObject {
 public:
  JSStringObject(Context* ctx, JSString* value)
    : JSObject(Map::NewUniqueMap(ctx)),
      value_(value),
      length_(static_cast<uint32_t>(value->size())) {
  }

  bool GetOwnPropertySlot(Context* ctx,
                          Symbol name, Slot* slot) const {
    if (name == symbol::length()) {
      slot->set_descriptor(
          DataDescriptor(JSVal::UInt32(length_), ATTR::NONE));
      return true;
    }
    if (symbol::IsArrayIndexSymbol(name)) {
      const uint32_t index = symbol::GetIndexFromSymbol(name);
      if (JSObject::GetOwnPropertySlot(ctx, name, slot)) {
        return true;
      }
      const std::size_t len = value_->size();
      if (len <= index) {
        return false;
      }
      slot->set_descriptor(
          DataDescriptor(
              JSString::NewSingle(ctx, value_->At(index)),
              ATTR::ENUMERABLE));
      return true;
    } else {
      return JSObject::GetOwnPropertySlot(ctx, name, slot);
    }
  }

  void GetOwnPropertyNames(Context* ctx,
                           PropertyNamesCollector* collector,
                           EnumerationMode mode) const {
    if (mode == INCLUDE_NOT_ENUMERABLE) {
      collector->Add(symbol::length(), 0);
    }
    for (uint32_t i = 0, len = value_->size(); i < len; ++i) {
      collector->Add(i);
    }
    JSObject::GetOwnPropertyNames(ctx, collector, mode);
  }

  JSString* value() const {
    return value_;
  }

  static JSStringObject* New(Context* ctx, JSString* str) {
    JSStringObject* const obj = new JSStringObject(ctx, str);
    obj->set_cls(JSStringObject::GetClass());
    obj->set_prototype(context::GetClassSlot(ctx, Class::String).prototype);
    return obj;
  }

  static JSStringObject* NewPlain(Context* ctx) {
    return new JSStringObject(ctx, JSString::NewEmptyString(ctx));
  }

  static const Class* GetClass() {
    static const Class cls = {
      "String",
      Class::String
    };
    return &cls;
  }

  void MarkChildren(radio::Core* core) {
    core->MarkCell(value_);
  }

 private:
  JSString* value_;
  uint32_t length_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSSTRINGOBJECT_H_
