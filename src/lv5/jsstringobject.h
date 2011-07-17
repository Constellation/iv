#ifndef _IV_LV5_JSSTRINGOBJECT_H_
#define _IV_LV5_JSSTRINGOBJECT_H_
#include "lv5/jsobject.h"
#include "lv5/jsstring.h"
#include "lv5/context_utils.h"
#include "lv5/symbol_checker.h"
namespace iv {
namespace lv5 {

class JSStringObject : public JSObject {
 public:
  JSStringObject(Context* ctx, JSString* value)
    : value_(value),
      length_(static_cast<uint32_t>(value->size())) {
  }

  PropertyDescriptor GetOwnProperty(Context* ctx, Symbol name) const {
    if (name == symbol::length) {
      return DataDescriptor(JSVal::UInt32(length_), PropertyDescriptor::NONE);
    }
    uint32_t index;
    if (core::ConvertToUInt32(context::GetSymbolString(ctx, name), &index)) {
      return JSStringObject::GetOwnPropertyWithIndex(ctx, index);
    }
    return JSObject::GetOwnProperty(ctx, name);
  }

  PropertyDescriptor GetOwnPropertyWithIndex(Context* ctx,
                                             uint32_t index) const {
    if (const SymbolChecker check = SymbolChecker(ctx, index)) {
      PropertyDescriptor desc = JSObject::GetOwnProperty(ctx, check.symbol());
      if (!desc.IsEmpty()) {
        return desc;
      }
    }
    const std::size_t len = value_->size();
    if (len <= index) {
      return JSUndefined;
    }
    return DataDescriptor(
        JSString::NewSingle(ctx, value_->GetAt(index)),
        PropertyDescriptor::ENUMERABLE);
  }

  void GetOwnPropertyNames(Context* ctx,
                           std::vector<Symbol>* vec,
                           EnumerationMode mode) const {
    if (mode == kIncludeNotEnumerable) {
      if (std::find(vec->begin(), vec->end(), symbol::length) == vec->end()) {
        vec->push_back(symbol::length);
      }
    }
    if (vec->empty()) {
      for (uint32_t i = 0, len = value_->size(); i < len; ++i) {
        vec->push_back(context::Intern(ctx, i));
      }
    } else {
      for (uint32_t i = 0, len = value_->size(); i < len; ++i) {
        const Symbol sym = context::Intern(ctx, i);
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
#endif  // _IV_LV5_JSSTRINGOBJECT_H_
