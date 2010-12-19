#ifndef _IV_LV5_INTERNAL_H_
#define _IV_LV5_INTERNAL_H_
#include "property.h"
#include "jsval.h"
#include "jsobject.h"
#include "context.h"
#include "error.h"
#include "lv5.h"
namespace iv {
namespace lv5 {

inline JSVal FromPropertyDescriptor(Context* ctx,
                                    const PropertyDescriptor& desc) {
  if (desc.IsEmpty()) {
    return JSUndefined;
  }
  JSObject* const obj = JSObject::New(ctx);
  if (desc.IsDataDescriptor()) {
    const DataDescriptor* const data = desc.AsDataDescriptor();
    obj->DefineOwnProperty(
        ctx, ctx->Intern("value"),
        DataDescriptor(data->value(),
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

inline PropertyDescriptor ToPropertyDescriptor(Context* ctx,
                                               const JSVal& target,
                                               Error* error) {
  if (!target.IsObject()) {
    error->Report(Error::Type,
                  "ToPropertyDescriptor requires Object argument");
    return JSUndefined;
  }
  int attr = PropertyDescriptor::kDefaultAttr;
  JSObject* const obj = target.object();
  JSVal value = JSUndefined;
  JSObject* getter = NULL;
  JSObject* setter = NULL;
  {
    // step 3
    const Symbol sym = ctx->Intern("enumerable");
    if (obj->HasProperty(ctx, sym)) {
      const JSVal r = obj->Get(ctx, sym, ERROR(error));
      const bool enumerable = r.ToBoolean(ERROR(error));
      if (enumerable) {
        attr = (attr & ~PropertyDescriptor::UNDEF_ENUMERABLE) |
            PropertyDescriptor::ENUMERABLE;
      } else {
        attr = (attr & ~PropertyDescriptor::UNDEF_CONFIGURABLE);
      }
    }
  }
  {
    // step 4
    const Symbol sym = ctx->Intern("configurable");
    if (obj->HasProperty(ctx, sym)) {
      const JSVal r = obj->Get(ctx, sym, ERROR(error));
      const bool configurable = r.ToBoolean(ERROR(error));
      if (configurable) {
        attr = (attr & ~PropertyDescriptor::UNDEF_CONFIGURABLE) |
            PropertyDescriptor::CONFIGURABLE;
      } else {
        attr = (attr & ~PropertyDescriptor::UNDEF_CONFIGURABLE);
      }
    }
  }
  {
    // step 5
    const Symbol sym = ctx->Intern("value");
    if (obj->HasProperty(ctx, sym)) {
      value = obj->Get(ctx, sym, ERROR(error));
      attr |= PropertyDescriptor::DATA;
      attr &= ~PropertyDescriptor::UNDEF_VALUE;
    }
  }
  {
    // step 6
    const Symbol sym = ctx->Intern("writable");
    if (obj->HasProperty(ctx, sym)) {
      const JSVal r = obj->Get(ctx, sym, ERROR(error));
      const bool writable = r.ToBoolean(ERROR(error));
      attr |= PropertyDescriptor::DATA;
      attr &= ~PropertyDescriptor::UNDEF_WRITABLE;
      if (writable) {
        attr |= PropertyDescriptor::WRITABLE;
      }
    }
  }
  {
    // step 7
    const Symbol sym = ctx->Intern("get");
    if (obj->HasProperty(ctx, sym)) {
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
    if (obj->HasProperty(ctx, sym)) {
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
    if (attr & PropertyDescriptor::DATA) {
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
    return GenericDescriptor(attr);
  }
}

inline bool StrictEqual(const JSVal& lhs, const JSVal& rhs) {
  if (lhs.type() != rhs.type()) {
    return false;
  }
  if (lhs.IsUndefined()) {
    return true;
  }
  if (lhs.IsNull()) {
    return true;
  }
  if (lhs.IsNumber()) {
    const double& lhsv = lhs.number();
    const double& rhsv = rhs.number();
    if (std::isnan(lhsv) || std::isnan(rhsv)) {
      return false;
    }
    return lhsv == rhsv;
  }
  if (lhs.IsString()) {
    return *(lhs.string()) == *(rhs.string());
  }
  if (lhs.IsBoolean()) {
    return lhs.boolean() == rhs.boolean();
  }
  if (lhs.IsObject()) {
    return lhs.object() == rhs.object();
  }
  return false;
}

#define ABSTRACT_CHECK\
  ERROR_WITH(error, false)
inline bool AbstractEqual(Context* ctx,
                          const JSVal& lhs, const JSVal& rhs,
                          Error* error) {
  if (lhs.type() == rhs.type()) {
    if (lhs.IsUndefined()) {
      return true;
    }
    if (lhs.IsNull()) {
      return true;
    }
    if (lhs.IsNumber()) {
      const double& lhsv = lhs.number();
      const double& rhsv = rhs.number();
      if (std::isnan(lhsv) || std::isnan(rhsv)) {
        return false;
      }
      return lhsv == rhsv;
    }
    if (lhs.IsString()) {
      return *(lhs.string()) == *(rhs.string());
    }
    if (lhs.IsBoolean()) {
      return lhs.boolean() == rhs.boolean();
    }
    if (lhs.IsObject()) {
      return lhs.object() == rhs.object();
    }
    return false;
  }
  if (lhs.IsNull() && rhs.IsUndefined()) {
    return true;
  }
  if (lhs.IsUndefined() && rhs.IsNull()) {
    return true;
  }
  if (lhs.IsNumber() && rhs.IsString()) {
    const double num = rhs.ToNumber(ctx, ABSTRACT_CHECK);
    return AbstractEqual(ctx, lhs, num, error);
  }
  if (lhs.IsString() && rhs.IsNumber()) {
    const double num = lhs.ToNumber(ctx, ABSTRACT_CHECK);
    return AbstractEqual(ctx, num, rhs, error);
  }
  if (lhs.IsBoolean()) {
    const double num = lhs.ToNumber(ctx, ABSTRACT_CHECK);
    return AbstractEqual(ctx, num, rhs, error);
  }
  if (rhs.IsBoolean()) {
    const double num = rhs.ToNumber(ctx, ABSTRACT_CHECK);
    return AbstractEqual(ctx, lhs, num, error);
  }
  if ((lhs.IsString() || lhs.IsNumber()) &&
      rhs.IsObject()) {
    const JSVal prim = rhs.ToPrimitive(ctx,
                                       Hint::NONE, ABSTRACT_CHECK);
    return AbstractEqual(ctx, lhs, prim, error);
  }
  if (lhs.IsObject() &&
      (rhs.IsString() || rhs.IsNumber())) {
    const JSVal prim = lhs.ToPrimitive(ctx,
                                       Hint::NONE, ABSTRACT_CHECK);
    return AbstractEqual(ctx, prim, rhs, error);
  }
  return false;
}
#undef ABSTRACT_CHECK

} }  // namespace iv::lv5
#endif  // _IV_LV5_INTERNAL_H_
