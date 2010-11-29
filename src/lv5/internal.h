#ifndef _IV_LV5_INTERNAL_H_
#define _IV_LV5_INTERNAL_H_
#include "property.h"
#include "jsval.h"
#include "jsobject.h"
#include "context.h"
namespace iv {
namespace lv5 {
JSVal FromPropertyDescriptor(Context* ctx, const PropertyDescriptor& desc) {
  if (desc.IsEmpty()) {
    return JSUndefined;
  }
  JSObject* const obj = JSObject::New(ctx);
  if (desc.IsDataDescriptor()) {
    const DataDescriptor* const data = desc.AsDataDescriptor();
    obj->DefineOwnProperty(
        ctx, ctx->Intern("value"),
        DataDescriptor(data->data(),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::ENUMERABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);
    obj->DefineOwnProperty(
        ctx, ctx->Intern("writable"),
        DataDescriptor(JSVal::Bool(data->IsWritable()),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::ENUMERABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);
  } else {
    assert(desc.IsAccessorDescriptor());
    const AccessorDescriptor* const accs = desc.AsAccessorDescriptor();
    const JSVal getter = (accs->get()) ? accs->get() : JSVal(JSUndefined);
    obj->DefineOwnProperty(
        ctx, ctx->Intern("get"),
        DataDescriptor(getter,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::ENUMERABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);
    const JSVal setter = (accs->set()) ? accs->set() : JSVal(JSUndefined);
    obj->DefineOwnProperty(
        ctx, ctx->Intern("set"),
        DataDescriptor(setter,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::ENUMERABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);
  }
  obj->DefineOwnProperty(
      ctx, ctx->Intern("enumerable"),
      DataDescriptor(JSVal::Bool(desc.IsEnumerable()),
                     PropertyDescriptor::WRITABLE |
                     PropertyDescriptor::ENUMERABLE |
                     PropertyDescriptor::CONFIGURABLE),
      false, NULL);
  obj->DefineOwnProperty(
      ctx, ctx->Intern("configurable"),
      DataDescriptor(JSVal::Bool(desc.IsConfigurable()),
                     PropertyDescriptor::WRITABLE |
                     PropertyDescriptor::ENUMERABLE |
                     PropertyDescriptor::CONFIGURABLE),
      false, NULL);
  return obj;
}
} }  // namespace iv::lv5
#endif  // _IV_LV5_INTERNAL_H_
