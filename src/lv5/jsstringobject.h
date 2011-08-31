#ifndef IV_LV5_JSSTRINGOBJECT_H_
#define IV_LV5_JSSTRINGOBJECT_H_
#include "lv5/jsobject.h"
#include "lv5/jsstring.h"
#include "lv5/map.h"
#include "lv5/slot.h"
#include "lv5/context_utils.h"
namespace iv {
namespace lv5 {

class JSStringObject : public JSObject {
 public:
  JSStringObject(Context* ctx, JSString* value)
    : JSObject(context::GetStringMap(ctx)),
      value_(value),
      length_(static_cast<uint32_t>(value->size())) {
  }

  bool GetOwnPropertySlot(Context* ctx,
                          Symbol name, Slot* slot) const {
    if (name == symbol::length()) {
      slot->set_descriptor(
          DataDescriptor(JSVal::UInt32(length_), PropertyDescriptor::NONE));
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
              JSString::NewSingle(ctx, value_->GetAt(index)),
              PropertyDescriptor::ENUMERABLE));
      return true;
    } else {
      return JSObject::GetOwnPropertySlot(ctx, name, slot);
    }
  }

  void GetOwnPropertyNames(Context* ctx,
                           std::vector<Symbol>* vec,
                           EnumerationMode mode) const {
    if (mode == kIncludeNotEnumerable) {
      if (std::find(vec->begin(), vec->end(), symbol::length()) == vec->end()) {
        vec->push_back(symbol::length());
      }
    }
    if (vec->empty()) {
      for (uint32_t i = 0, len = value_->size(); i < len; ++i) {
        vec->push_back(symbol::MakeSymbolFromIndex(i));
      }
    } else {
      for (uint32_t i = 0, len = value_->size(); i < len; ++i) {
        const Symbol sym = symbol::MakeSymbolFromIndex(i);
        if (std::find(vec->begin(), vec->end(), sym) == vec->end()) {
          vec->push_back(sym);
        }
      }
    }
    JSObject::GetOwnPropertyNames(ctx, vec, mode);
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

 private:
  JSString* value_;
  uint32_t length_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSSTRINGOBJECT_H_
