#ifndef IV_LV5_RUNTIME_REFLECT_H_
#define IV_LV5_RUNTIME_REFLECT_H_
#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/internal.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/context.h>
#include <iv/lv5/internal.h>

namespace iv {
namespace lv5 {
namespace runtime {

// 15.17.1.1 Reflect.getPrototypeOf(target)
inline JSVal ReflectGetPrototypeOf(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Reflect.getPrototypeOf", args, e);
  const JSVal target = args.At(0);
  Context* ctx = args.ctx();
  JSObject* obj = target.ToObject(ctx, IV_LV5_ERROR(e));
  return obj->prototype();
}

// 15.17.1.2 Reflect.setPrototypeOf(target, proto)
inline JSVal ReflectSetPrototypeOf(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Reflect.setPrototypeOf", args, e);
  const JSVal target = args.At(0);
  const JSVal proto = args.At(1);
  Context* ctx = args.ctx();
  JSObject* obj = target.ToObject(ctx, IV_LV5_ERROR(e));
  if (!proto.IsObject() && !proto.IsNull()) {
    e->Report(Error::Type, "second parameter `proto` should be null or object");
    return JSEmpty;
  }
  if (!obj->IsExtensible()) {
    return JSFalse;
  }
  obj->ChangePrototype(ctx, proto.IsObject() ? proto.object() : NULL);
  return JSTrue;
}

// 15.17.1.3 Reflect.isExtensible(target)
inline JSVal ReflectIsExtensible(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Reflect.isExtensible", args, e);
  const JSVal target = args.At(0);
  Context* ctx = args.ctx();
  JSObject* obj = target.ToObject(ctx, IV_LV5_ERROR(e));
  return JSVal::Bool(obj->IsExtensible());
}

// 15.17.1.4 Reflect.preventExtensions(target)
inline JSVal ReflectPreventExtensions(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Reflect.preventExtensions", args, e);
  const JSVal target = args.At(0);
  Context* ctx = args.ctx();
  JSObject* obj = target.ToObject(ctx, IV_LV5_ERROR(e));
  obj->ChangeExtensible(ctx, false);
  return JSUndefined;
}

// 15.17.1.5 Reflect.hasOwn(target, propertyKey)
inline JSVal ReflectHasOwn(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Reflect.hasOwn", args, e);
  const JSVal target = args.At(0);
  const JSVal property = args.At(1);
  Context* ctx = args.ctx();
  JSObject* obj = target.ToObject(ctx, IV_LV5_ERROR(e));
  const Symbol key = property.ToSymbol(ctx, IV_LV5_ERROR(e));
  return JSVal::Bool(obj->HasOwnProperty(ctx, key));
}

// 15.17.1.6 Reflect.getOwnPropertyDescriptor(target, propertyKey)
inline JSVal ReflectGetOwnPropertyDescriptor(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Reflect.getOwnPropertyDescriptor", args, e);
  const JSVal target = args.At(0);
  const JSVal property = args.At(1);
  Context* ctx = args.ctx();
  JSObject* obj = target.ToObject(ctx, IV_LV5_ERROR(e));
  const Symbol key = property.ToSymbol(ctx, IV_LV5_ERROR(e));
  const PropertyDescriptor desc = obj->GetOwnProperty(ctx, key);
  return internal::FromPropertyDescriptor(ctx, desc);
}

// 15.17.1.7 Reflect.get(target, propertyKey, receiver=target)
#if 0
inline JSVal ReflectGet(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Reflect.get", args, e);
  const JSVal target = args.At(0);
  const JSVal property = args.At(1);
  Context* ctx = args.ctx();
  JSObject* obj = target.ToObject(ctx, IV_LV5_ERROR(e));
  const Symbol key = property.ToSymbol(ctx, IV_LV5_ERROR(e));

  JSVal receiver = target;
  if (args.size() > 2) {
    receiver = args.At(2);
  }

  // TODO(Constellation) we should define [[GetP]] and use receiver
  return obj->Get(ctx, key, e);
}
#endif

// 15.17.1.8 Reflect.set(target, propertyKey, V, receiver=target)
#if 0
inline JSVal ReflectSet(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Reflect.set", args, e);
  const JSVal target = args.At(0);
  const JSVal property = args.At(1);
  Context* ctx = args.ctx();
  JSObject* obj = target.ToObject(ctx, IV_LV5_ERROR(e));
  const Symbol key = property.ToSymbol(ctx, IV_LV5_ERROR(e));

  JSVal receiver = target;
  if (args.size() > 2) {
    receiver = args.At(2);
  }

  // TODO(Constellation) we should define [[SetP]] and use receiver
  return obj->Get(ctx, key, e);
}
#endif

// 15.17.1.9 Reflect.deleteProperty(target, propertyKey)
inline JSVal ReflectDeleteProperty(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Reflect.deleteProperty", args, e);
  const JSVal target = args.At(0);
  const JSVal property = args.At(1);
  Context* ctx = args.ctx();
  JSObject* obj = target.ToObject(ctx, IV_LV5_ERROR(e));
  const Symbol key = property.ToSymbol(ctx, IV_LV5_ERROR(e));
  return JSVal::Bool(obj->Delete(ctx, key, true, e));
}

// 15.17.1.10 Reflect.defineProperty(target, propertyKey, attributes)
inline JSVal ReflectDefineProperty(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Reflect.defineProperty", args, e);
  const JSVal target = args.At(0);
  const JSVal property = args.At(1);
  Context* ctx = args.ctx();
  JSObject* obj = target.ToObject(ctx, IV_LV5_ERROR(e));
  const Symbol key = property.ToSymbol(ctx, IV_LV5_ERROR(e));
  const JSVal attributes = args.At(2);
  const PropertyDescriptor desc =
      internal::ToPropertyDescriptor(ctx, attributes, IV_LV5_ERROR(e));
  return JSVal::Bool(
      obj->DefineOwnProperty(ctx, key, desc, true, e));
}

// 15.17.1.11 Reflect.enumerate(target)
#if 0
inline JSVal ReflectEnumerate(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Reflect.enumerate", args, e);
  const JSVal target = args.At(0);
  Context* ctx = args.ctx();
  JSObject* obj = target.ToObject(ctx, IV_LV5_ERROR(e));
  return obj->Enumerate(ctx, e);
}
#endif

// 15.17.1.12 Reflect.keys(target)
inline JSVal ReflectKeys(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Reflect.keys", args, e);
  const JSVal target = args.At(0);
  Context* ctx = args.ctx();
  JSObject* obj = target.ToObject(ctx, IV_LV5_ERROR(e));
  PropertyNamesCollector collector;
  obj->GetOwnPropertyNames(ctx, &collector, EXCLUDE_NOT_ENUMERABLE);
  JSVector* const vec = JSVector::New(ctx, collector.names().size());
  JSVector::iterator res = vec->begin();
  for (PropertyNamesCollector::Names::const_iterator
       it = collector.names().begin(),
       last = collector.names().end();
       it != last; ++it, ++res) {
    *res = JSString::New(ctx, *it);
  }
  return vec->ToJSArray();
}

// 15.17.1.13 Reflect.getOwnPropertyNames(target)
inline JSVal ReflectGetOwnPropertyNames(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Reflect.getOwnPropertyNames", args, e);
  const JSVal target = args.At(0);
  Context* ctx = args.ctx();
  JSObject* obj = target.ToObject(ctx, IV_LV5_ERROR(e));
  PropertyNamesCollector collector;
  obj->GetOwnPropertyNames(ctx, &collector, INCLUDE_NOT_ENUMERABLE);
  JSVector* const vec = JSVector::New(ctx, collector.names().size());
  JSVector::iterator res = vec->begin();
  for (PropertyNamesCollector::Names::const_iterator
       it = collector.names().begin(),
       last = collector.names().end();
       it != last; ++it, ++res) {
    *res = JSString::New(ctx, *it);
  }
  return vec->ToJSArray();
}

// 15.17.1.14 Reflect.freeze(target)
inline JSVal ReflectFreeze(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Reflect.freeze", args, e);
  const JSVal target = args.At(0);
  Context* ctx = args.ctx();
  JSObject* obj = target.ToObject(ctx, IV_LV5_ERROR(e));
  obj->Freeze(ctx, e);
  return JSTrue;
}

// 15.17.1.14 Reflect.seal(target)
inline JSVal ReflectSeal(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Reflect.seal", args, e);
  const JSVal target = args.At(0);
  Context* ctx = args.ctx();
  JSObject* obj = target.ToObject(ctx, IV_LV5_ERROR(e));
  obj->Seal(ctx, e);
  return JSTrue;
}

// 15.17.1.16 Reflect.isFrozen(target)
inline JSVal ReflectIsFrozen(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Reflect.isFrozen", args, e);
  const JSVal target = args.At(0);
  Context* ctx = args.ctx();
  JSObject* obj = target.ToObject(ctx, IV_LV5_ERROR(e));
  PropertyNamesCollector collector;
  obj->GetOwnPropertyNames(ctx, &collector, INCLUDE_NOT_ENUMERABLE);
  for (PropertyNamesCollector::Names::const_iterator
       it = collector.names().begin(),
       last = collector.names().end();
       it != last; ++it) {
    const PropertyDescriptor desc = obj->GetOwnProperty(ctx, *it);
    if (desc.IsData() && desc.AsDataDescriptor()->IsWritable()) {
      return JSFalse;
    }
    if (desc.IsConfigurable()) {
      return JSFalse;
    }
  }
  return JSVal::Bool(!obj->IsExtensible());
}

// 15.17.1.17 Reflect.isSealed(target)
inline JSVal ReflectIsSealed(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Reflect.isSealed", args, e);
  const JSVal target = args.At(0);
  Context* ctx = args.ctx();
  JSObject* obj = target.ToObject(ctx, IV_LV5_ERROR(e));
  PropertyNamesCollector collector;
  obj->GetOwnPropertyNames(ctx, &collector, INCLUDE_NOT_ENUMERABLE);
  for (PropertyNamesCollector::Names::const_iterator
       it = collector.names().begin(),
       last = collector.names().end();
       it != last; ++it) {
    const PropertyDescriptor desc = obj->GetOwnProperty(ctx, *it);
    if (desc.IsConfigurable()) {
      return JSFalse;
    }
  }
  return JSVal::Bool(!obj->IsExtensible());
}

// 15.17.1.18 Reflect.has(target, propertyKey)
inline JSVal ReflectHas(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Reflect.has", args, e);
  const JSVal target = args.At(0);
  const JSVal property = args.At(1);
  Context* ctx = args.ctx();
  JSObject* obj = target.ToObject(ctx, IV_LV5_ERROR(e));
  const Symbol key = property.ToSymbol(ctx, IV_LV5_ERROR(e));
  return JSVal::Bool(obj->HasProperty(ctx, key));
}

// 15.17.2.2 Reflect.instanceOf(target, O)
inline JSVal ReflectInstanceOf(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Reflect.instanceOf", args, e);
  const JSVal target = args.At(0);
  const JSVal o = args.At(1);
  Context* ctx = args.ctx();
  if (!o.IsObject()) {
    e->Report(Error::Type, "instanceof requires object");
    return JSEmpty;
  }
  JSObject* const rhs = o.object();
  if (!rhs->IsCallable()) {
    e->Report(Error::Type, "instanceof requires constructor");
    return JSEmpty;
  }
  return JSVal::Bool(static_cast<JSFunction*>(rhs)->HasInstance(ctx, target, e));
}

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_REFLECT_H_
