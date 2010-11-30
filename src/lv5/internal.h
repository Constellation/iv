#ifndef _IV_LV5_INTERNAL_H_
#define _IV_LV5_INTERNAL_H_
#include "property.h"
#include "jsval.h"
#include "jsobject.h"
#include "context.h"
#include "lv5.h"
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

PropertyDescriptor ToPropertyDescriptor(Context* ctx,
                                        const JSVal& target, Error* error) {
  if (!target.IsObject()) {
    error->Report(Error::Type,
                  "ToPropertyDescriptor requires Object argument");
    return JSUndefined;
  }
  int attr = PropertyDescriptor::kDefaultAttr;
  JSObject* const obj = target.object();
  JSVal value;
  JSObject* getter = NULL;
  JSObject* setter = NULL;
  {
    // step 3
    const Symbol sym = ctx->Intern("enumerable");
    if (obj->HasProperty(sym)) {
      const JSVal r = obj->Get(ctx, sym, ERROR(error));
      const bool enumerable = r.ToBoolean(ERROR(error));
      if (enumerable) {
        attr = (attr & ~PropertyDescriptor::UNDEF_ENUMERABLE) | PropertyDescriptor::ENUMERABLE;
      }
    }
  }
  {
    // step 4
    const Symbol sym = ctx->Intern("configurable");
    if (obj->HasProperty(sym)) {
      const JSVal r = obj->Get(ctx, sym, ERROR(error));
      const bool configurable = r.ToBoolean(ERROR(error));
      if (configurable) {
        attr = (attr & ~PropertyDescriptor::UNDEF_CONFIGURABLE) | PropertyDescriptor::CONFIGURABLE;
      }
    }
  }
  {
    // step 5
    const Symbol sym = ctx->Intern("value");
    if (obj->HasProperty(sym)) {
      value = obj->Get(ctx, sym, ERROR(error));
      attr |= PropertyDescriptor::DATA;
    }
  }
  {
    // step 6
    const Symbol sym = ctx->Intern("writable");
    if (obj->HasProperty(sym)) {
      const JSVal r = obj->Get(ctx, sym, ERROR(error));
      const bool writable = r.ToBoolean(ERROR(error));
      if (writable) {
        attr = (attr & ~PropertyDescriptor::UNDEF_WRITABLE) | PropertyDescriptor::WRITABLE;
      }
    }
  }
  {
    // step 7
    const Symbol sym = ctx->Intern("get");
    if (obj->HasProperty(sym)) {
      const JSVal r = obj->Get(ctx, sym, ERROR(error));
      if (!r.IsCallable() && !r.IsUndefined()) {
        error->Report(Error::Type,
                      "property \"get\" is not callable");
        return JSUndefined;
      }
      attr |= PropertyDescriptor::ACCESSOR;
      if (!r.IsUndefined()) {
        getter = r.object();
      }
    }
  }
  {
    // step 8
    const Symbol sym = ctx->Intern("set");
    if (obj->HasProperty(sym)) {
      const JSVal r = obj->Get(ctx, sym, ERROR(error));
      if (!r.IsCallable() && !r.IsUndefined()) {
        error->Report(Error::Type,
                      "property \"set\" is not callable");
        return JSUndefined;
      }
      attr |= PropertyDescriptor::ACCESSOR;
      if (!r.IsUndefined()) {
        setter = r.object();
      }
    }
  }
  // step 9
  if (attr & PropertyDescriptor::ACCESSOR) {
    if ((attr & PropertyDescriptor::DATA) ||
        (attr & PropertyDescriptor::WRITABLE)) {
      error->Report(Error::Type,
                    "invalid object for property descriptor");
      return JSUndefined;
    }
  }
  if (attr & PropertyDescriptor::ACCESSOR) {
    return AccessorDescriptor(getter, setter, attr);
  } else if (attr & PropertyDescriptor::DATA) {
    return DataDescriptor(value, attr);
  } else {
    return PropertyDescriptor(attr);
  }
}

} }  // namespace iv::lv5
#endif  // _IV_LV5_INTERNAL_H_
