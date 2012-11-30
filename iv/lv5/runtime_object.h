#ifndef IV_LV5_RUNTIME_OBJECT_H_
#define IV_LV5_RUNTIME_OBJECT_H_
#include <cstddef>
#include <vector>
#include <utility>
#include <string>
#include <algorithm>
#include <iv/detail/array.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsstring.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsarray.h>
#include <iv/lv5/error.h>
#include <iv/lv5/context.h>
#include <iv/lv5/context.h>
#include <iv/lv5/internal.h>

namespace iv {
namespace lv5 {
namespace runtime {
namespace detail {

void DefinePropertiesHelper(Context* ctx, JSObject* obj,
                            JSObject* props, Error* e);

std::unordered_set<core::UString> CreateNativeBrandSet();
const std::unordered_set<core::UString>& NativeBrandSet();

}  // namespace detail

// section 15.2.1.1 Object([value])
// section 15.2.2.1 new Object([value])
inline JSVal ObjectConstructor(const Arguments& args, Error* e) {
  if (args.IsConstructorCalled()) {
    const JSVal val = args.At(0);
    if (val.IsObject()) {
      JSObject* const obj = val.object();
      if (obj->IsNativeObject()) {
        return obj;
      } else {
        // 15.2.2.1 step 1.a.ii
        // implementation dependent host object behavior
        return JSUndefined;
      }
    }
    if (val.IsString() || val.IsBoolean() || val.IsNumber()) {
      return val.ToObject(args.ctx(), e);
    }
    assert(val.IsNullOrUndefined());
    return JSObject::New(args.ctx());
  } else {
    const JSVal val = args.At(0);
    if (val.IsNullOrUndefined()) {
      return JSObject::New(args.ctx());
    } else {
      return val.ToObject(args.ctx(), e);
    }
  }
}

// section 15.2.3.2 Object.getPrototypeOf(O)
inline JSVal ObjectGetPrototypeOf(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Object.getPrototypeOf", args, e);
  if (!args.empty()) {
    const JSVal first = args.front();
    if (first.IsObject()) {
      JSObject* const obj = first.object()->prototype();
      if (obj) {
        return obj;
      } else {
        return JSNull;
      }
    }
  }
  e->Report(Error::Type, "Object.getPrototypeOf requires Object argument");
  return JSUndefined;
}

// section 15.2.3.3 Object.getOwnPropertyDescriptor(O, P)
inline JSVal ObjectGetOwnPropertyDescriptor(const Arguments& args,
                                            Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Object.getOwnPropertyDescriptor", args, e);
  if (!args.empty()) {
    const JSVal first = args.front();
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      const Symbol name = args.At(1).ToSymbol(args.ctx(), IV_LV5_ERROR(e));
      const PropertyDescriptor desc = obj->GetOwnProperty(args.ctx(), name);
      return internal::FromPropertyDescriptor(args.ctx(), desc);
    }
  }
  e->Report(Error::Type,
            "Object.getOwnPropertyDescriptor requires Object argument");
  return JSUndefined;
}

// section 15.2.3.4 Object.getOwnPropertyNames(O)
inline JSVal ObjectGetOwnPropertyNames(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Object.getOwnPropertyNames", args, e);
  if (!args.empty()) {
    const JSVal first = args.front();
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      Context* const ctx = args.ctx();
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
  }
  e->Report(Error::Type, "Object.getOwnPropertyNames requires Object argument");
  return JSUndefined;
}

inline void detail::DefinePropertiesHelper(Context* ctx,
                                           JSObject* obj,
                                           JSObject* props, Error* e) {
  typedef trace::Vector<PropertyDescriptor>::type Descriptors;
  PropertyNamesCollector collector;
  props->GetOwnPropertyNames(ctx, &collector, EXCLUDE_NOT_ENUMERABLE);
  Descriptors descriptors(collector.names().size());
  {
    Descriptors::iterator descs = descriptors.begin();
    for (PropertyNamesCollector::Names::const_iterator
         it = collector.names().begin(),
         last = collector.names().end();
         it != last; ++it, ++descs) {
      const Symbol sym = *it;
      const JSVal desc_obj = props->Get(ctx, sym, IV_LV5_ERROR_VOID(e));
      *descs =
          internal::ToPropertyDescriptor(ctx, desc_obj, IV_LV5_ERROR_VOID(e));
    }
  }
  {
    Descriptors::const_iterator descs = descriptors.begin();
    for (PropertyNamesCollector::Names::const_iterator
         it = collector.names().begin(),
         last = collector.names().end();
         it != last; ++it, ++descs) {
      obj->DefineOwnProperty(ctx, *it, *descs, true, IV_LV5_ERROR_VOID(e));
    }
  }
}

// section 15.2.3.5 Object.create(O[, Properties])
inline JSVal ObjectCreate(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Object.create", args, e);
  if (!args.empty()) {
    const JSVal first = args.front();
    if (first.IsObject() || first.IsNull()) {
      JSObject* prototype = NULL;
      if (first.IsObject()) {
        prototype = first.object();
      }
      JSObject* const res = JSObject::New(args.ctx(), Map::NewUniqueMap(args.ctx(), prototype));
      const JSVal second = args.At(1);
      if (!second.IsUndefined()) {
        JSObject* const props = second.ToObject(args.ctx(), IV_LV5_ERROR(e));
        detail::DefinePropertiesHelper(args.ctx(), res, props, IV_LV5_ERROR(e));
      }
      return res;
    }
  }
  e->Report(Error::Type, "Object.create requires Object or Null argument");
  return JSUndefined;
}

// section 15.2.3.6 Object.defineProperty(O, P, Attributes)
inline JSVal ObjectDefineProperty(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Object.defineProperty", args, e);
  if (!args.empty()) {
    const JSVal first = args.front();
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      const Symbol name = args.At(1).ToSymbol(args.ctx(), IV_LV5_ERROR(e));
      const JSVal attr = args.At(2);
      const PropertyDescriptor desc =
          internal::ToPropertyDescriptor(args.ctx(), attr, IV_LV5_ERROR(e));
      obj->DefineOwnProperty(args.ctx(), name, desc, true, IV_LV5_ERROR(e));
      return obj;
    }
  }
  e->Report(Error::Type, "Object.defineProperty requires Object argument");
  return JSUndefined;
}

// section 15.2.3.7 Object.defineProperties(O, Properties)
inline JSVal ObjectDefineProperties(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Object.defineProperties", args, e);
  if (!args.empty()) {
    const JSVal first = args.front();
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      JSObject* const props = args.At(1).ToObject(args.ctx(), IV_LV5_ERROR(e));
      detail::DefinePropertiesHelper(args.ctx(), obj, props, IV_LV5_ERROR(e));
      return obj;
    }
  }
  e->Report(Error::Type, "Object.defineProperties requires Object argument");
  return JSUndefined;
}

// section 15.2.3.8 Object.seal(O)
inline JSVal ObjectSeal(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Object.seal", args, e);
  Context* const ctx = args.ctx();
  if (!args.empty()) {
    const JSVal first = args.front();
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      obj->Seal(ctx, IV_LV5_ERROR(e));
      return obj;
    }
  }
  e->Report(Error::Type, "Object.seal requires Object argument");
  return JSUndefined;
}

// section 15.2.3.9 Object.freeze(O)
inline JSVal ObjectFreeze(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Object.freeze", args, e);
  Context* const ctx = args.ctx();
  if (!args.empty()) {
    const JSVal first = args.front();
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      obj->Freeze(ctx, IV_LV5_ERROR(e));
      return obj;
    }
  }
  e->Report(Error::Type, "Object.freeze requires Object argument");
  return JSUndefined;
}

// section 15.2.3.10 Object.preventExtensions(O)
inline JSVal ObjectPreventExtensions(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Object.preventExtensions", args, e);
  if (!args.empty()) {
    const JSVal first = args.front();
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      obj->ChangeExtensible(args.ctx(), false);
      return obj;
    }
  }
  e->Report(Error::Type, "Object.preventExtensions requires Object argument");
  return JSUndefined;
}

// section 15.2.3.11 Object.isSealed(O)
inline JSVal ObjectIsSealed(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Object.isSealed", args, e);
  Context* const ctx = args.ctx();
  if (!args.empty()) {
    const JSVal first = args.front();
    if (first.IsObject()) {
      JSObject* const obj = first.object();
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
  }
  e->Report(Error::Type, "Object.isSealed requires Object argument");
  return JSUndefined;
}

// section 15.2.3.12 Object.isFrozen(O)
inline JSVal ObjectIsFrozen(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Object.isFrozen", args, e);
  Context* const ctx = args.ctx();
  if (!args.empty()) {
    const JSVal first = args.front();
    if (first.IsObject()) {
      JSObject* const obj = first.object();
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
  }
  e->Report(Error::Type, "Object.isFrozen requires Object argument");
  return JSUndefined;
}

// section 15.2.3.13 Object.isExtensible(O)
inline JSVal ObjectIsExtensible(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Object.isExtensible", args, e);
  if (!args.empty()) {
    const JSVal first = args.front();
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      return JSVal::Bool(obj->IsExtensible());
    }
  }
  e->Report(Error::Type, "Object.isExtensible requires Object argument");
  return JSUndefined;
}

// section 15.2.3.14 Object.keys(O)
inline JSVal ObjectKeys(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Object.keys", args, e);
  Context* const ctx = args.ctx();
  if (!args.empty()) {
    const JSVal first = args.front();
    if (first.IsObject()) {
      JSObject* const obj = first.object();
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
  }
  e->Report(Error::Type, "Object.keys requires Object argument");
  return JSUndefined;
}

// ES.next Object.is(x, y)
// http://wiki.ecmascript.org/doku.php?id=harmony:egal
inline JSVal ObjectIs(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Object.is", args, e);
  return JSVal::Bool(JSVal::SameValue(args.At(0), args.At(1)));
}

inline std::unordered_set<core::UString> detail::CreateNativeBrandSet() {
  std::unordered_set<core::UString> set;
  set.insert(core::ToUString("Arguments"));
  set.insert(core::ToUString("Array"));
  set.insert(core::ToUString("Boolean"));
  set.insert(core::ToUString("Date"));
  set.insert(core::ToUString("Error"));
  set.insert(core::ToUString("Function"));
  set.insert(core::ToUString("JSON"));
  set.insert(core::ToUString("Math"));
  set.insert(core::ToUString("Number"));
  set.insert(core::ToUString("Object"));
  set.insert(core::ToUString("RegExp"));
  set.insert(core::ToUString("String"));
  return set;
}

inline const std::unordered_set<core::UString>& detail::NativeBrandSet() {
  static const std::unordered_set<core::UString>
      set(detail::CreateNativeBrandSet());
  return set;
}

// section 15.2.4.2 Object.prototype.toString()
inline JSVal ObjectToString(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Object.prototype.toString", args, e);
  const JSVal this_binding = args.this_binding();
  if (this_binding.IsUndefined()) {
    return JSString::NewAsciiString(args.ctx(), "[object Undefined]", e);
  }
  if (this_binding.IsNull()) {
    return JSString::NewAsciiString(args.ctx(), "[object Null]", e);
  }

  JSObject* const obj = this_binding.ToObject(args.ctx(), IV_LV5_ERROR(e));
  JSStringBuilder builder;
  builder.Append("[object ");
  builder.Append(obj->cls()->name);
  builder.Append("]");
  return builder.Build(args.ctx(), false, e);
}

// section 15.2.4.3 Object.prototype.toLocaleString()
inline JSVal ObjectToLocaleString(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Object.prototype.toLocaleString", args, e);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  const JSVal toString = obj->Get(ctx, symbol::toString(), IV_LV5_ERROR(e));
  if (!toString.IsCallable()) {
    e->Report(Error::Type, "toString is not callable");
    return JSUndefined;
  }
  ScopedArguments arguments(ctx, 0, IV_LV5_ERROR(e));
  return static_cast<JSFunction*>(toString.object())->Call(&arguments, obj, e);
}

// section 15.2.4.4 Object.prototype.valueOf()
inline JSVal ObjectValueOf(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Object.prototype.valueOf", args, e);
  JSObject* const obj =
      args.this_binding().ToObject(args.ctx(), IV_LV5_ERROR(e));
  if (obj->IsNativeObject()) {
    return obj;
  } else {
    // 15.2.2.1 step 1.a.ii
    // 15.2.4.4 step 2.a
    // implementation dependent host object behavior
    return JSUndefined;
  }
}

// section 15.2.4.5 Object.prototype.hasOwnProperty(V)
inline JSVal ObjectHasOwnProperty(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Object.prototype.hasOwnProperty", args, e);
  if (!args.empty()) {
    const JSVal val = args.front();
    Context* const ctx = args.ctx();
    const Symbol name = val.ToSymbol(ctx, IV_LV5_ERROR(e));
    JSObject* const obj = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
    return JSVal::Bool(!obj->GetOwnProperty(ctx, name).IsEmpty());
  } else {
    return JSFalse;
  }
}

// section 15.2.4.6 Object.prototype.isPrototypeOf(V)
inline JSVal ObjectIsPrototypeOf(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Object.prototype.isPrototypeOf", args, e);
  if (!args.empty()) {
    const JSVal first = args.front();
    if (first.IsObject()) {
      JSObject* const v = first.object();
      JSObject* const obj =
          args.this_binding().ToObject(args.ctx(), IV_LV5_ERROR(e));
      JSObject* proto = v->prototype();
      while (proto) {
        if (obj == proto) {
          return JSTrue;
        }
        proto = proto->prototype();
      }
    }
  }
  return JSFalse;
}

// section 15.2.4.7 Object.prototype.propertyIsEnumerable(V)
inline JSVal ObjectPropertyIsEnumerable(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Object.prototype.propertyIsEnumerable", args, e);
  Symbol name = args.At(0).ToSymbol(args.ctx(), IV_LV5_ERROR(e));
  JSObject* const obj =
      args.this_binding().ToObject(args.ctx(), IV_LV5_ERROR(e));
  const PropertyDescriptor desc = obj->GetOwnProperty(args.ctx(), name);
  return JSVal::Bool(!desc.IsEmpty() && desc.IsEnumerable());
}

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_OBJECT_H_
