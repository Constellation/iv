#ifndef _IV_LV5_RUNTIME_OBJECT_H_
#define _IV_LV5_RUNTIME_OBJECT_H_
#include <cstddef>
#include <vector>
#include <utility>
#include <string>
#include <algorithm>
#include <tr1/array>
#include "lv5/lv5.h"
#include "lv5/arguments.h"
#include "lv5/jsval.h"
#include "lv5/jsstring.h"
#include "lv5/jsobject.h"
#include "lv5/jsarray.h"
#include "lv5/error.h"
#include "lv5/context.h"
#include "lv5/internal.h"

namespace iv {
namespace lv5 {
namespace runtime {
namespace detail {

class IsEnumerable {
 public:
  template<typename T>
  inline bool operator()(const T& val) const {
    return val.second.IsEnumerable();
  }
};

inline void DefinePropertiesImpl(Context* ctx,
                                 JSObject* obj,
                                 JSObject* props, Error* error) {
  typedef std::vector<std::pair<Symbol, PropertyDescriptor> > Descriptors;
  Descriptors descriptors;
  std::vector<Symbol> keys;
  props->GetOwnPropertyNames(ctx, &keys, JSObject::kExcludeNotEnumerable);
  for (std::vector<Symbol>::const_iterator it = keys.begin(),
       last = keys.end(); it != last; ++it) {
    const JSVal desc_obj = props->Get(ctx, *it,
                                      ERROR_VOID(error));
    const PropertyDescriptor desc =
        ToPropertyDescriptor(ctx, desc_obj, ERROR_VOID(error));
    descriptors.push_back(std::make_pair(*it, desc));
  }
  for (Descriptors::const_iterator it = descriptors.begin(),
       last = descriptors.end(); it != last; ++it) {
    obj->DefineOwnProperty(ctx, it->first, it->second, true, ERROR_VOID(error));
  }
}

}  // namespace iv::lv5::runtime::detail

// section 15.2.1.1 Object([value])
// section 15.2.2.1 new Object([value])
inline JSVal ObjectConstructor(const Arguments& args, Error* error) {
  if (args.IsConstructorCalled()) {
    if (args.size() > 0) {
      const JSVal& val = args[0];
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
      if (val.IsString() ||
          val.IsBoolean() ||
          val.IsNumber()) {
        return val.ToObject(args.ctx(), error);
      }
      assert(val.IsNull() || val.IsUndefined());
    }
    return JSObject::New(args.ctx());
  } else {
    if (args.size() > 0) {
      const JSVal& val = args[0];
      if (val.IsNull() || val.IsUndefined()) {
        return JSObject::New(args.ctx());
      } else {
        return val.ToObject(args.ctx(), error);
      }
    } else {
      return JSObject::New(args.ctx());
    }
  }
}

// section 15.2.3.2 Object.getPrototypeOf(O)
inline JSVal ObjectGetPrototypeOf(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.getPrototypeOf", args, error);
  if (args.size() > 0) {
    const JSVal& first = args[0];
    if (first.IsObject()) {
      JSObject* const obj = first.object()->prototype();
      if (obj) {
        return obj;
      } else {
        return JSNull;
      }
    }
  }
  error->Report(Error::Type,
                "Object.getPrototypeOf requires Object argument");
  return JSUndefined;
}

// section 15.2.3.3 Object.getOwnPropertyDescriptor(O, P)
inline JSVal ObjectGetOwnPropertyDescriptor(const Arguments& args,
                                            Error* error) {
  CONSTRUCTOR_CHECK("Object.getOwnPropertyDescriptor", args, error);
  if (args.size() > 0) {
    const JSVal& first = args[0];
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      Symbol name;
      if (args.size() > 1) {
        JSString* const str = args[1].ToString(args.ctx(), ERROR(error));
        name = args.ctx()->Intern(str->value());
      } else {
        name = args.ctx()->Intern("undefined");
      }
      const PropertyDescriptor desc = obj->GetOwnProperty(args.ctx(), name);
      return FromPropertyDescriptor(args.ctx(), desc);
    }
  }
  error->Report(Error::Type,
                "Object.getOwnPropertyDescriptor requires Object argument");
  return JSUndefined;
}

// section 15.2.3.4 Object.getOwnPropertyNames(O)
inline JSVal ObjectGetOwnPropertyNames(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.getOwnPropertyNames", args, error);
  if (args.size() > 0) {
    const JSVal& first = args[0];
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      JSArray* const ary = JSArray::New(args.ctx());
      Context* const ctx = args.ctx();
      uint32_t n = 0;
      std::vector<Symbol> keys;
      obj->GetOwnPropertyNames(ctx, &keys, JSObject::kIncludeNotEnumerable);
      for (std::vector<Symbol>::const_iterator it = keys.begin(),
           last = keys.end(); it != last; ++it, ++n) {
        ary->DefineOwnPropertyWithIndex(
            args.ctx(), n,
            DataDescriptor(ctx->ToString(*it),
                           PropertyDescriptor::WRITABLE |
                           PropertyDescriptor::ENUMERABLE |
                           PropertyDescriptor::CONFIGURABLE),
            false, ERROR(error));
      }
      return ary;
    }
  }
  error->Report(Error::Type,
                "Object.getOwnPropertyNames requires Object argument");
  return JSUndefined;
}

// section 15.2.3.5 Object.create(O[, Properties])
inline JSVal ObjectCreate(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.create", args, error);
  if (args.size() > 0) {
    const JSVal& first = args[0];
    if (first.IsObject()) {
      JSObject* const res = JSObject::New(args.ctx());
      JSObject* const obj = first.object();
      res->set_prototype(obj);
      if (args.size() > 1 && !args[1].IsUndefined()) {
        JSObject* const props = args[1].ToObject(args.ctx(), ERROR(error));
        detail::DefinePropertiesImpl(args.ctx(), res, props, ERROR(error));
      }
      return res;
    }
  }
  error->Report(Error::Type,
                "Object.create requires Object argument");
  return JSUndefined;
}

// section 15.2.3.6 Object.defineProperty(O, P, Attributes)
inline JSVal ObjectDefineProperty(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.defineProperty", args, error);
  if (args.size() > 0) {
    const JSVal& first = args[0];
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      Symbol name;
      if (args.size() > 1) {
        const JSString* const str = args[1].ToString(args.ctx(), ERROR(error));
        name = args.ctx()->Intern(str->value());
      } else {
        name = args.ctx()->Intern("undefined");
      }
      JSVal attr = JSUndefined;
      if (args.size() > 2) {
        attr = args[2];
      }
      const PropertyDescriptor desc = ToPropertyDescriptor(args.ctx(),
                                                           attr,
                                                           ERROR(error));
      obj->DefineOwnProperty(args.ctx(), name, desc, true, ERROR(error));
      return obj;
    }
  }
  error->Report(Error::Type,
                "Object.defineProperty requires Object argument");
  return JSUndefined;
}

// section 15.2.3.7 Object.defineProperties(O, Properties)
inline JSVal ObjectDefineProperties(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.defineProperties", args, error);
  if (args.size() > 0) {
    const JSVal& first = args[0];
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      if (args.size() > 1) {
        JSObject* const props = args[1].ToObject(args.ctx(), ERROR(error));
        detail::DefinePropertiesImpl(args.ctx(), obj, props, ERROR(error));
        return obj;
      } else {
        // raise TypeError
        JSVal(JSUndefined).ToObject(args.ctx(), ERROR(error));
        return JSUndefined;
      }
    }
  }
  error->Report(Error::Type,
                "Object.defineProperties requires Object argument");
  return JSUndefined;
}

// section 15.2.3.8 Object.seal(O)
inline JSVal ObjectSeal(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.seal", args, error);
  if (args.size() > 0) {
    const JSVal& first = args[0];
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      std::vector<Symbol> keys;
      Context* const ctx = args.ctx();
      obj->GetOwnPropertyNames(ctx, &keys, JSObject::kIncludeNotEnumerable);
      for (std::vector<Symbol>::const_iterator it = keys.begin(),
           last = keys.end(); it != last; ++it) {
        PropertyDescriptor desc = obj->GetOwnProperty(ctx, *it);
        if (desc.IsConfigurable()) {
          desc.set_configurable(false);
        }
        obj->DefineOwnProperty(
            ctx, *it, desc, true, ERROR(error));
      }
      obj->set_extensible(false);
      return obj;
    }
  }
  error->Report(Error::Type,
                "Object.seal requires Object argument");
  return JSUndefined;
}

// section 15.2.3.9 Object.freeze(O)
inline JSVal ObjectFreeze(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.freeze", args, error);
  if (args.size() > 0) {
    const JSVal& first = args[0];
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      std::vector<Symbol> keys;
      Context* const ctx = args.ctx();
      obj->GetOwnPropertyNames(ctx, &keys, JSObject::kIncludeNotEnumerable);
      for (std::vector<Symbol>::const_iterator it = keys.begin(),
           last = keys.end(); it != last; ++it) {
        PropertyDescriptor desc = obj->GetOwnProperty(ctx, *it);
        if (desc.IsDataDescriptor()) {
          desc.AsDataDescriptor()->set_writable(false);
        }
        if (desc.IsConfigurable()) {
          desc.set_configurable(false);
        }
        obj->DefineOwnProperty(
            ctx, *it, desc, true, ERROR(error));
      }
      obj->set_extensible(false);
      return obj;
    }
  }
  error->Report(Error::Type,
                "Object.freeze requires Object argument");
  return JSUndefined;
}

// section 15.2.3.10 Object.preventExtensions(O)
inline JSVal ObjectPreventExtensions(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.preventExtensions", args, error);
  if (args.size() > 0) {
    const JSVal& first = args[0];
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      obj->set_extensible(false);
      return obj;
    }
  }
  error->Report(Error::Type,
                "Object.preventExtensions requires Object argument");
  return JSUndefined;
}

// section 15.2.3.11 Object.isSealed(O)
inline JSVal ObjectIsSealed(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.isSealed", args, error);
  if (args.size() > 0) {
    const JSVal& first = args[0];
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      std::vector<Symbol> keys;
      Context* const ctx = args.ctx();
      obj->GetOwnPropertyNames(ctx, &keys, JSObject::kIncludeNotEnumerable);
      for (std::vector<Symbol>::const_iterator it = keys.begin(),
           last = keys.end(); it != last; ++it) {
        const PropertyDescriptor desc = obj->GetOwnProperty(ctx, *it);
        if (desc.IsConfigurable()) {
          return JSFalse;
        }
      }
      return JSVal::Bool(!obj->IsExtensible());
    }
  }
  error->Report(Error::Type,
                "Object.isSealed requires Object argument");
  return JSUndefined;
}

// section 15.2.3.12 Object.isFrozen(O)
inline JSVal ObjectIsFrozen(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.isFrozen", args, error);
  if (args.size() > 0) {
    const JSVal& first = args[0];
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      std::vector<Symbol> keys;
      Context* const ctx = args.ctx();
      obj->GetOwnPropertyNames(ctx, &keys, JSObject::kIncludeNotEnumerable);
      for (std::vector<Symbol>::const_iterator it = keys.begin(),
           last = keys.end(); it != last; ++it) {
        const PropertyDescriptor desc = obj->GetOwnProperty(ctx, *it);
        if (desc.IsDataDescriptor()) {
          if (desc.AsDataDescriptor()->IsWritable()) {
            return JSFalse;
          }
        }
        if (desc.IsConfigurable()) {
          return JSFalse;
        }
      }
      return JSVal::Bool(!obj->IsExtensible());
    }
  }
  error->Report(Error::Type,
                "Object.isFrozen requires Object argument");
  return JSUndefined;
}

// section 15.2.3.13 Object.isExtensible(O)
inline JSVal ObjectIsExtensible(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.isExtensible", args, error);
  if (args.size() > 0) {
    const JSVal& first = args[0];
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      return JSVal::Bool(obj->IsExtensible());
    }
  }
  error->Report(Error::Type,
                "Object.isExtensible requires Object argument");
  return JSUndefined;
}

// section 15.2.3.14 Object.keys(O)
inline JSVal ObjectKeys(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.keys", args, error);
  if (args.size() > 0) {
    const JSVal& first = args[0];
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      Context* const ctx = args.ctx();
      std::vector<Symbol> keys;
      obj->GetOwnPropertyNames(ctx, &keys, JSObject::kExcludeNotEnumerable);
      JSArray* const ary = JSArray::New(args.ctx(), keys.size());
      uint32_t index = 0;
      for (std::vector<Symbol>::const_iterator it = keys.begin(),
           last = keys.end(); it != last; ++it, ++index) {
        ary->DefineOwnPropertyWithIndex(
            ctx, index,
            DataDescriptor(
                args.ctx()->ToString(*it),
                PropertyDescriptor::WRITABLE |
                PropertyDescriptor::ENUMERABLE |
                PropertyDescriptor::CONFIGURABLE),
            false, ERROR(error));
      }
      return ary;
    }
  }
  error->Report(Error::Type,
                "Object.keys requires Object argument");
  return JSUndefined;
}

// section 15.2.4.2 Object.prototype.toString()
inline JSVal ObjectToString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.prototype.toString", args, error);
  const JSVal& this_binding = args.this_binding();
  if (this_binding.IsUndefined()) {
    return JSString::NewAsciiString(args.ctx(), "[object Undefined]");
  }
  if (this_binding.IsNull()) {
    return JSString::NewAsciiString(args.ctx(), "[object Null]");
  }
  JSObject* const obj = this_binding.ToObject(args.ctx(), ERROR(error));
  StringBuilder builder;
  builder.Append("[object ");
  builder.Append(context::GetSymbolString(args.ctx(), obj->class_name()));
  builder.Append("]");
  return builder.Build(args.ctx());
}

// section 15.2.4.3 Object.prototype.toLocaleString()
inline JSVal ObjectToLocaleString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.prototype.toLocaleString", args, error);
  JSObject* const obj = args.this_binding().ToObject(args.ctx(), ERROR(error));
  const JSVal toString = obj->Get(args.ctx(),
                                  args.ctx()->toString_symbol(), ERROR(error));
  if (!toString.IsCallable()) {
    error->Report(Error::Type,
                  "toString is not callable");
    return JSUndefined;
  }
  Arguments arguments(args.ctx(), 0);
  return toString.object()->AsCallable()->Call(arguments, obj, error);
}

// section 15.2.4.4 Object.prototype.valueOf()
inline JSVal ObjectValueOf(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.prototype.valueOf", args, error);
  JSObject* const obj = args.this_binding().ToObject(args.ctx(), ERROR(error));
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
inline JSVal ObjectHasOwnProperty(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.prototype.hasOwnProperty", args, error);
  if (args.size() > 0) {
    const JSVal& val = args[0];
    Context* const ctx = args.ctx();
    JSString* const str = val.ToString(ctx, ERROR(error));
    JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(error));
    if (!obj->GetOwnProperty(ctx, ctx->Intern(str->value())).IsEmpty()) {
      return JSTrue;
    } else {
      return JSFalse;
    }
  } else {
    return JSFalse;
  }
}

// section 15.2.4.6 Object.prototype.isPrototypeOf(V)
inline JSVal ObjectIsPrototypeOf(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.prototype.isPrototypeOf", args, error);
  if (args.size() > 0) {
    const JSVal& first = args[0];
    if (first.IsObject()) {
      JSObject* const v = first.object();
      JSObject* const obj =
          args.this_binding().ToObject(args.ctx(), ERROR(error));
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
inline JSVal ObjectPropertyIsEnumerable(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.prototype.propertyIsEnumerable", args, error);
  Symbol name;
  if (args.size() > 0) {
    const JSString* const str = args[0].ToString(args.ctx(), ERROR(error));
    name = args.ctx()->Intern(str->value());
  } else {
    name = args.ctx()->Intern("undefined");
  }
  JSObject* const obj = args.this_binding().ToObject(args.ctx(), ERROR(error));
  const PropertyDescriptor desc = obj->GetOwnProperty(args.ctx(), name);
  if (desc.IsEmpty()) {
    return JSFalse;
  }
  return JSVal::Bool(desc.IsEnumerable());
}

} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_OBJECT_H_
