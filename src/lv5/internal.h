#ifndef _IV_LV5_INTERNAL_H_
#define _IV_LV5_INTERNAL_H_
#include <tr1/memory>
#include "parser.h"
#include "lv5/lv5.h"
#include "lv5/eval_source.h"
#include "lv5/factory.h"
#include "lv5/property.h"
#include "lv5/jsscript.h"
#include "lv5/jsstring.h"
#include "lv5/jsval.h"
#include "lv5/jsobject.h"
#include "lv5/context.h"
#include "lv5/error.h"
namespace iv {
namespace lv5 {

inline JSScript* CompileScript(Context* ctx, const JSString* str,
                               bool is_strict, Error* error) {
  std::tr1::shared_ptr<EvalSource> const src(new EvalSource(*str));
  AstFactory* const factory = new AstFactory(ctx);
  core::Parser<AstFactory, EvalSource, true, true> parser(factory, src.get());
  parser.set_strict(is_strict);
  const FunctionLiteral* const eval = parser.ParseProgram();
  if (!eval) {
    delete factory;
    error->Report(Error::Syntax,
                  parser.error());
    return NULL;
  } else {
    return JSEvalScript<EvalSource>::New(ctx, eval, factory, src);
  }
}

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
        attr = (attr & ~PropertyDescriptor::UNDEF_ENUMERABLE);
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
      attr &= ~PropertyDescriptor::UNDEF_GETTER;
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
      attr &= ~PropertyDescriptor::UNDEF_SETTER;
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

enum CompareKind {
  CMP_TRUE,
  CMP_FALSE,
  CMP_UNDEFINED,
  CMP_ERROR
};

// section 11.8.5
template<bool LeftFirst>
CompareKind Compare(Context* ctx,
                    const JSVal& lhs, const JSVal& rhs, Error* e) {
  JSVal px;
  JSVal py;
  if (LeftFirst) {
    px = lhs.ToPrimitive(ctx, Hint::NUMBER, e);
    if (*e) {
      return CMP_ERROR;
    }
    py = rhs.ToPrimitive(ctx, Hint::NUMBER, e);
    if (*e) {
      return CMP_ERROR;
    }
  } else {
    py = rhs.ToPrimitive(ctx, Hint::NUMBER, e);
    if (*e) {
      return CMP_ERROR;
    }
    px = lhs.ToPrimitive(ctx, Hint::NUMBER, e);
    if (*e) {
      return CMP_ERROR;
    }
  }
  if (px.IsString() && py.IsString()) {
    // step 4
    return (*(px.string()) < *(py.string())) ? CMP_TRUE : CMP_FALSE;
  } else {
    const double nx = px.ToNumber(ctx, e);
    if (*e) {
      return CMP_ERROR;
    }
    const double ny = py.ToNumber(ctx, e);
    if (*e) {
      return CMP_ERROR;
    }
    if (std::isnan(nx) || std::isnan(ny)) {
      return CMP_UNDEFINED;
    }
    if (nx == ny) {
      if (std::signbit(nx) != std::signbit(ny)) {
        return CMP_FALSE;
      }
      return CMP_FALSE;
    }
    if (nx == std::numeric_limits<double>::infinity()) {
      return CMP_FALSE;
    }
    if (ny == std::numeric_limits<double>::infinity()) {
      return CMP_TRUE;
    }
    if (ny == (-std::numeric_limits<double>::infinity())) {
      return CMP_FALSE;
    }
    if (nx == (-std::numeric_limits<double>::infinity())) {
      return CMP_TRUE;
    }
    return (nx < ny) ? CMP_TRUE : CMP_FALSE;
  }
}

} }  // namespace iv::lv5
#endif  // _IV_LV5_INTERNAL_H_
