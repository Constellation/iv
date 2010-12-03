#include <iostream>  // NOLINT
#include <gc/gc.h>
#include <tr1/memory>
#include "lv5.h"
#include "error.h"
#include "jserror.h"
#include "jsarray.h"
#include "runtime.h"
#include "factory.h"
#include "context.h"
#include "eval-source.h"
#include "parser.h"
#include "internal.h"
namespace iv {
namespace lv5 {
namespace {

const std::string function_prefix("function ");
const std::string error_split(": ");
const double kInfinity = std::numeric_limits<double>::infinity();

static JSString* ErrorMessageString(const Arguments& args, Error* error) {
  if (args.size() > 0) {
    const JSVal& msg = args[0];
    if (!msg.IsUndefined()) {
      return msg.ToString(args.ctx(), ERROR_WITH(error, NULL));
    } else {
      return JSString::NewEmptyString(args.ctx());
    }
  } else {
    return JSString::NewEmptyString(args.ctx());
  }
}

static JSScript* CompileScript(Context* ctx, JSString* str, Error* error) {
  std::tr1::shared_ptr<EvalSource> const src(new EvalSource(str));
  AstFactory* const factory = new AstFactory(ctx);
  core::Parser<AstFactory, EvalSource> parser(factory, src.get());
  parser.set_strict(ctx->IsStrict());
  const iv::lv5::FunctionLiteral* const eval = parser.ParseProgram();
  if (!eval) {
    delete factory;
    error->Report(Error::Syntax,
                  parser.error());
    return NULL;
  } else {
    return iv::lv5::JSEvalScript<EvalSource>::New(ctx, eval, factory, src);
  }
}

}  // namespace

JSVal Runtime_GlobalEval(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("eval", args, error);
  return Runtime_InDirectCallToEval(args, error);
}

JSVal Runtime_DirectCallToEval(const Arguments& args, Error* error) {
  if (!args.size()) {
    return JSUndefined;
  }
  const JSVal& first = args[0];
  if (!first.IsString()) {
    return first;
  }
  Context* const ctx = args.ctx();
  JSScript* const script = CompileScript(args.ctx(),
                                         first.string(), ERROR(error));
  if (script->function()->strict()) {
    JSDeclEnv* const env =
        Interpreter::NewDeclarativeEnvironment(ctx, ctx->lexical_env());
    const Interpreter::ContextSwitcher switcher(ctx,
                                                env,
                                                env,
                                                ctx->this_binding(),
                                                true);
    ctx->Run(script);
  } else {
    const Interpreter::ContextSwitcher switcher(ctx,
                                                ctx->global_env(),
                                                ctx->global_env(),
                                                ctx->global_obj(),
                                                false);
    ctx->Run(script);
  }
  if (ctx->IsShouldGC()) {
    GC_gcollect();
  }
  return ctx->ret();
}

JSVal Runtime_InDirectCallToEval(const Arguments& args, Error* error) {
  if (!args.size()) {
    return JSUndefined;
  }
  const JSVal& first = args[0];
  if (!first.IsString()) {
    return first;
  }
  Context* const ctx = args.ctx();
  JSScript* const script = CompileScript(args.ctx(),
                                         first.string(), ERROR(error));
  if (script->function()->strict()) {
    JSDeclEnv* const env =
        Interpreter::NewDeclarativeEnvironment(ctx, ctx->lexical_env());
    const Interpreter::ContextSwitcher switcher(ctx,
                                                env,
                                                env,
                                                ctx->this_binding(),
                                                true);
    ctx->Run(script);
  } else {
    ctx->Run(script);
  }
  if (ctx->IsShouldGC()) {
    GC_gcollect();
  }
  return ctx->ret();
}

JSVal Runtime_GlobalParseInt(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("parseInt", args, error);
  if (args.size() > 0) {
    JSString* const str = args[0].ToString(args.ctx(), ERROR(error));
    int radix = 0;
    if (args.size() > 1) {
      const double ret = args[1].ToNumber(args.ctx(), ERROR(error));
      radix = core::DoubleToInt32(ret);
    }
    bool strip_prefix = true;
    if (radix != 0) {
      if (radix < 2 || radix > 36) {
        return JSNaN;
      }
      if (radix != 16) {
        strip_prefix = false;
      }
    } else {
      radix = 10;
    }
    return core::StringToIntegerWithRadix(str->begin(), str->end(),
                                          radix,
                                          strip_prefix);
  } else {
    return JSNaN;
  }
}

JSVal Runtime_GlobalParseFloat(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("parseFloat", args, error);
  if (args.size() > 0) {
    JSString* const str = args[0].ToString(args.ctx(), ERROR(error));
    return core::StringToDouble(str->value(), true);
  } else {
    return JSNaN;
  }
}

JSVal Runtime_GlobalIsNaN(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("isNaN", args, error);
  if (args.size() > 0) {
    const double number = args[0].ToNumber(args.ctx(), ERROR(error));
    if (std::isnan(number)) {
      return JSTrue;
    } else {
      return JSFalse;
    }
  } else {
    return JSTrue;
  }
}

JSVal Runtime_GlobalIsFinite(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("isFinite", args, error);
  if (args.size() > 0) {
    const double number = args[0].ToNumber(args.ctx(), ERROR(error));
    if (std::isfinite(number)) {
      return JSTrue;
    } else {
      return JSFalse;
    }
  } else {
    return JSTrue;
  }
}

JSVal Runtime_ThrowTypeError(const Arguments& args, Error* error) {
  error->Report(Error::Type,
                "[[ThrowTypeError]] called");
  return JSUndefined;
}

// section 15.2.1.1 Object([value])
// section 15.2.2.1 new Object([value])
JSVal Runtime_ObjectConstructor(const Arguments& args, Error* error) {
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
JSVal Runtime_ObjectGetPrototypeOf(const Arguments& args, Error* error) {
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
JSVal Runtime_ObjectGetOwnPropertyDescriptor(const Arguments& args,
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
      const PropertyDescriptor desc = obj->GetOwnProperty(name);
      return FromPropertyDescriptor(args.ctx(), desc);
    }
  }
  error->Report(Error::Type,
                "Object.getOwnPropertyDescriptor requires Object argument");
  return JSUndefined;
}

// section 15.2.3.4 Object.getOwnPropertyNames(O)
JSVal Runtime_ObjectGetOwnPropertyNames(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.getOwnPropertyNames", args, error);
  if (args.size() > 0) {
    const JSVal& first = args[0];
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      JSArray* const ary = JSArray::New(args.ctx());
      std::size_t n = 0;
      std::tr1::array<char, 80> buffer;
      for (JSObject::Properties::const_iterator it = obj->table().begin(),
           last = obj->table().end(); it != last; ++it, ++n) {
        const char* const str = core::DoubleToCString(n,
                                                      buffer.data(),
                                                      buffer.size());
        ary->DefineOwnProperty(
            args.ctx(), args.ctx()->Intern(str),
            DataDescriptor(args.ctx()->ToString(it->first),
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

void DefinePropertiesImpl(Context* ctx,
                          JSObject* obj,
                          JSObject* props, Error* error) {
  typedef std::vector<std::pair<Symbol, PropertyDescriptor> > Descriptors;
  Descriptors descriptors;
  for (JSObject::Properties::const_iterator it = props->table().begin(),
       last = props->table().end(); it != last; ++it) {
    if (it->second.IsEnumerable()) {
      const JSVal desc_obj = props->Get(ctx, it->first,
                                        ERROR_VOID(error));
      const PropertyDescriptor desc =
          ToPropertyDescriptor(ctx, desc_obj, ERROR_VOID(error));
      descriptors.push_back(std::make_pair(it->first, desc));
    }
  }
  for (Descriptors::const_iterator it = descriptors.begin(),
       last = descriptors.end(); it != last; ++it) {
    obj->DefineOwnProperty(ctx, it->first, it->second, true, ERROR_VOID(error));
  }
}

// section 15.2.3.5 Object.create(O[, Properties])
JSVal Runtime_ObjectCreate(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.create", args, error);
  if (args.size() > 0) {
    const JSVal& first = args[0];
    if (first.IsObject()) {
      JSObject* const res = JSObject::New(args.ctx());
      JSObject* const obj = first.object();
      res->set_prototype(obj);
      if (args.size() > 1 && !args[1].IsUndefined()) {
        JSObject* const props = args[1].ToObject(args.ctx(), ERROR(error));
        DefinePropertiesImpl(args.ctx(), res, props, ERROR(error));
      }
      return res;
    }
  }
  error->Report(Error::Type,
                "Object.create requires Object argument");
  return JSUndefined;
}

// section 15.2.3.6 Object.defineProperty(O, P, Attributes)
JSVal Runtime_ObjectDefineProperty(const Arguments& args, Error* error) {
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
JSVal Runtime_ObjectDefineProperties(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.defineProperties", args, error);
  if (args.size() > 0) {
    const JSVal& first = args[0];
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      if (args.size() > 1) {
        JSObject* const props = args[1].ToObject(args.ctx(), ERROR(error));
        DefinePropertiesImpl(args.ctx(), obj, props, ERROR(error));
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
JSVal Runtime_ObjectSeal(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.seal", args, error);
  if (args.size() > 0) {
    const JSVal& first = args[0];
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      // TODO(Constellation) Enumerate Iterator Interface
      // (such as Arguments.length)
      for (JSObject::Properties::const_iterator it = obj->table().begin(),
           last = obj->table().end(); it != last; ++it) {
        PropertyDescriptor desc = obj->GetOwnProperty(it->first);
        if (desc.IsConfigurable()) {
          desc.SetConfigurable(false);
        }
        obj->DefineOwnProperty(
            args.ctx(), it->first, desc, true, ERROR(error));
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
JSVal Runtime_ObjectFreeze(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.freeze", args, error);
  if (args.size() > 0) {
    const JSVal& first = args[0];
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      // TODO(Constellation) Enumerate Iterator Interface
      // (such as Arguments.length)
      for (JSObject::Properties::const_iterator it = obj->table().begin(),
           last = obj->table().end(); it != last; ++it) {
        PropertyDescriptor desc = obj->GetOwnProperty(it->first);
        if (desc.IsDataDescriptor()) {
          desc.SetWritable(false);
        }
        if (desc.IsConfigurable()) {
          desc.SetConfigurable(false);
        }
        obj->DefineOwnProperty(
            args.ctx(), it->first, desc, true, ERROR(error));
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
JSVal Runtime_ObjectPreventExtensions(const Arguments& args, Error* error) {
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
JSVal Runtime_ObjectIsSealed(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.isSealed", args, error);
  if (args.size() > 0) {
    const JSVal& first = args[0];
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      // TODO(Constellation) Enumerate Iterator Interface
      // (such as Arguments.length)
      for (JSObject::Properties::const_iterator it = obj->table().begin(),
           last = obj->table().end(); it != last; ++it) {
        const PropertyDescriptor desc = obj->GetOwnProperty(it->first);
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
JSVal Runtime_ObjectIsFrozen(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.isFrozen", args, error);
  if (args.size() > 0) {
    const JSVal& first = args[0];
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      // TODO(Constellation) Enumerate Iterator Interface
      // (such as Arguments.length)
      for (JSObject::Properties::const_iterator it = obj->table().begin(),
           last = obj->table().end(); it != last; ++it) {
        const PropertyDescriptor desc = obj->GetOwnProperty(it->first);
        if (desc.IsDataDescriptor()) {
          if (desc.IsWritable()) {
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
JSVal Runtime_ObjectIsExtensible(const Arguments& args, Error* error) {
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

class IsEnumerable {
 public:
  template<typename T>
  bool operator()(const T& val) const {
    return val.second.IsEnumerable();
  }
};

// section 15.2.3.14 Object.keys(O)
JSVal Runtime_ObjectKeys(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.keys", args, error);
  if (args.size() > 0) {
    const JSVal& first = args[0];
    if (first.IsObject()) {
      JSObject* const obj = first.object();
      // TODO(Constellation) Enumerate Iterator Interface
      // (such as Arguments.length)
      const std::size_t n = std::count_if(obj->table().begin(),
                                          obj->table().end(),
                                          IsEnumerable());
      JSArray* const ary = JSArray::New(args.ctx(), n);
      std::size_t index = 0;
      for (JSObject::Properties::const_iterator it = obj->table().begin(),
           last = obj->table().end(); it != last; ++it, ++index) {
        if (it->second.IsEnumerable()) {
          JSString* const str = JSVal(index).ToString(args.ctx(), ERROR(error));
          ary->DefineOwnProperty(args.ctx(), args.ctx()->Intern(str->value()),
                                 DataDescriptor(
                                     args.ctx()->ToString(it->first),
                                     PropertyDescriptor::WRITABLE |
                                     PropertyDescriptor::ENUMERABLE |
                                     PropertyDescriptor::CONFIGURABLE),
                                 false, ERROR(error));
        }
      }
      return ary;
    }
  }
  error->Report(Error::Type,
                "Object.keys requires Object argument");
  return JSUndefined;
}

// section 15.2.4.2 Object.prototype.toString()
JSVal Runtime_ObjectToString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.prototype.toString", args, error);
  std::string ascii;
  JSObject* const obj = args.this_binding().ToObject(args.ctx(), ERROR(error));
  JSString* const cls = obj->cls();
  assert(cls);
  std::string str("[object ");
  str.append(cls->begin(), cls->end());
  str.append("]");
  return JSString::NewAsciiString(args.ctx(), str.c_str());
}

// section 15.2.4.3 Object.prototype.toLocaleString()
JSVal Runtime_ObjectToLocaleString(const Arguments& args, Error* error) {
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
  arguments.set_this_binding(obj);
  return toString.object()->AsCallable()->Call(arguments, error);
}

// section 15.2.4.4 Object.prototype.valueOf()
JSVal Runtime_ObjectValueOf(const Arguments& args, Error* error) {
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
JSVal Runtime_ObjectHasOwnProperty(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.prototype.hasOwnProperty", args, error);
  if (args.size() > 0) {
    const JSVal& val = args[0];
    Context* ctx = args.ctx();
    JSString* const str = val.ToString(ctx, ERROR(error));
    JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(error));
    if (!obj->GetOwnProperty(ctx->Intern(str->value())).IsEmpty()) {
      return JSTrue;
    } else {
      return JSFalse;
    }
  } else {
    return JSFalse;
  }
}

// section 15.2.4.6 Object.prototype.isPrototypeOf(V)
JSVal Runtime_ObjectIsPrototypeOf(const Arguments& args, Error* error) {
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
JSVal Runtime_ObjectPropertyIsEnumerable(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Object.prototype.propertyIsEnumerable", args, error);
  Symbol name;
  if (args.size() > 0) {
    const JSString* const str = args[0].ToString(args.ctx(), ERROR(error));
    name = args.ctx()->Intern(str->value());
  } else {
    name = args.ctx()->Intern("undefined");
  }
  JSObject* const obj = args.this_binding().ToObject(args.ctx(), ERROR(error));
  const PropertyDescriptor desc = obj->GetOwnProperty(name);
  if (desc.IsEmpty()) {
    return JSFalse;
  }
  return JSVal::Bool(desc.IsEnumerable());
}

JSVal Runtime_FunctionPrototype(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Function.prototype", args, error);
  return JSUndefined;
}

JSVal Runtime_FunctionToString(const Arguments& args,
                               Error* error) {
  CONSTRUCTOR_CHECK("Function.prototype.toString", args, error);
  const JSVal& obj = args.this_binding();
  if (obj.IsCallable()) {
    JSFunction* const func = obj.object()->AsCallable();
    if (func->AsNativeFunction()) {
      return JSString::NewAsciiString(args.ctx(),
                                      "function () { [native code] }");
    } else {
      core::UString buffer(function_prefix.begin(),
                           function_prefix.end());
      if (func->AsCodeFunction()->name()) {
        const core::UStringPiece name = func->AsCodeFunction()->name()->value();
        buffer.append(name.data(), name.size());
      }
      const core::UStringPiece src = func->AsCodeFunction()->GetSource();
      buffer.append(src.data(), src.size());
      return JSString::New(args.ctx(), buffer);
    }
  }
  error->Report(Error::Type,
                "Function.prototype.toString is not generic function");
  return JSUndefined;
}

// section 15.4.1.1 Array([item0 [, item1 [, ...]]])
// section 15.4.2.1 new Array([item0 [, item1 [, ...]]])
// section 15.4.2.2 new Array(len)
JSVal Runtime_ArrayConstructor(const Arguments& args, Error* error) {
// TODO(Constellation) this is mock function
  return JSArray::New(args.ctx());
}

JSVal Runtime_ErrorConstructor(const Arguments& args, Error* error) {
  JSString* message = ErrorMessageString(args, ERROR(error));
  return JSError::New(args.ctx(), Error::User, message);
}

JSVal Runtime_NativeErrorConstructor(const Arguments& args, Error* error) {
  JSString* message = ErrorMessageString(args, ERROR(error));
  return JSError::NewSyntaxError(args.ctx(), message);
}

JSVal Runtime_EvalErrorConstructor(const Arguments& args, Error* error) {
  JSString* message = ErrorMessageString(args, ERROR(error));
  return JSError::NewEvalError(args.ctx(), message);
}

JSVal Runtime_RangeErrorConstructor(const Arguments& args, Error* error) {
  JSString* message = ErrorMessageString(args, ERROR(error));
  return JSError::NewRangeError(args.ctx(), message);
}

JSVal Runtime_ReferenceErrorConstructor(const Arguments& args, Error* error) {
  JSString* message = ErrorMessageString(args, ERROR(error));
  return JSError::NewReferenceError(args.ctx(), message);
}

JSVal Runtime_SyntaxErrorConstructor(const Arguments& args, Error* error) {
  JSString* message = ErrorMessageString(args, ERROR(error));
  return JSError::NewSyntaxError(args.ctx(), message);
}

JSVal Runtime_TypeErrorConstructor(const Arguments& args, Error* error) {
  JSString* message = ErrorMessageString(args, ERROR(error));
  return JSError::NewTypeError(args.ctx(), message);
}

JSVal Runtime_URIErrorConstructor(const Arguments& args, Error* error) {
  JSString* message = ErrorMessageString(args, ERROR(error));
  return JSError::NewURIError(args.ctx(), message);
}

JSVal Runtime_ErrorToString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Error.prototype.toString", args, error);
  const JSVal& obj = args.this_binding();
  Context* const ctx = args.ctx();
  if (obj.IsObject()) {
    JSString* name;
    {
      const JSVal target = obj.object()->Get(ctx,
                                             ctx->Intern("name"),
                                             ERROR(error));
      if (target.IsUndefined()) {
        name = JSString::NewAsciiString(ctx, "Error");
      } else {
        name = target.ToString(ctx, ERROR(error));
      }
    }
    JSString* msg;
    {
      const JSVal target = obj.object()->Get(ctx,
                                             ctx->Intern("message"),
                                             ERROR(error));
      if (target.IsUndefined()) {
        return JSUndefined;
      } else {
        msg = target.ToString(ctx, ERROR(error));
      }
    }
    core::UString buffer;
    buffer.append(name->data(), name->size());
    buffer.append(error_split.begin(), error_split.end());
    buffer.append(msg->data(), msg->size());
    return JSString::New(ctx, buffer);
  }
  error->Report(Error::Type, "base must be object");
  return JSUndefined;
}

// section 15.5.1
JSVal Runtime_StringConstructor(const Arguments& args, Error* error) {
  if (args.IsConstructorCalled()) {
    JSString* str;
    if (args.size() > 0) {
      str = args[0].ToString(args.ctx(), ERROR(error));
    } else {
      str = JSString::NewEmptyString(args.ctx());
    }
    return JSStringObject::New(args.ctx(), str);
  } else {
    if (args.size() > 0) {
      return args[0].ToString(args.ctx(), error);
    } else {
      return JSString::NewEmptyString(args.ctx());
    }
  }
}

// section 15.5.3.2 String.fromCharCode([char0 [, char1[, ...]]])
JSVal Runtime_StringFromCharCode(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.fromCharCode", args, error);
  std::vector<uc16> buf(args.size());
  std::vector<uc16>::iterator target = buf.begin();
  for (Arguments::const_iterator it = args.begin(),
       last = args.end(); it != last; ++it, ++target) {
    const double val = it->ToNumber(args.ctx(), ERROR(error));
    *target = core::DoubleToUInt32(val);
  }
  return JSString::New(args.ctx(), core::UStringPiece(buf.data(), buf.size()));
}

static JSVal StringToStringValueOfImpl(const Arguments& args,
                                       Error* error,
                                       const char* msg) {
  const JSVal& obj = args.this_binding();
  if (!obj.IsString()) {
    if (obj.IsObject() && obj.object()->AsStringObject()) {
      return obj.object()->AsStringObject()->value();
    } else {
      error->Report(Error::Type, msg);
      return JSUndefined;
    }
  } else {
    return obj.string();
  }
}

// section 15.5.4.2 String.prototype.toString()
JSVal Runtime_StringToString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.toString", args, error);
  return StringToStringValueOfImpl(
      args, error,
      "String.prototype.toString is not generic function");
}

// section 15.5.4.3 String.prototype.valueOf()
JSVal Runtime_StringValueOf(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.valueOf", args, error);
  return StringToStringValueOfImpl(
      args, error,
      "String.prototype.valueOf is not generic function");
}

// section 15.5.4.4 String.prototype.charAt(pos)
JSVal Runtime_StringCharAt(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.charAt", args, error);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(error));
  const JSString* const str = val.ToString(args.ctx(), ERROR(error));
  double position;
  if (args.size() > 0) {
    position = args[0].ToNumber(args.ctx(), ERROR(error));
    position = core::DoubleToInteger(position);
  } else {
    // undefined -> NaN -> 0
    position = 0;
  }
  if (position < 0 || position >= str->size()) {
    return JSString::NewEmptyString(args.ctx());
  } else {
    return JSString::New(
        args.ctx(),
        core::UStringPiece(str->data() + core::DoubleToUInt32(position), 1));
  }
}

// section 15.5.4.5 String.prototype.charCodeAt(pos)
JSVal Runtime_StringCharCodeAt(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.charCodeAt", args, error);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(error));
  const JSString* const str = val.ToString(args.ctx(), ERROR(error));
  double position;
  if (args.size() > 0) {
    position = args[0].ToNumber(args.ctx(), ERROR(error));
    position = core::DoubleToInteger(position);
  } else {
    // undefined -> NaN -> 0
    position = 0;
  }
  if (position < 0 || position >= str->size()) {
    return JSNaN;
  } else {
    return (*str)[core::DoubleToUInt32(position)];
  }
}

// section 15.5.4.6 String.prototype.concat([string1[, string2[, ...]]])
JSVal Runtime_StringConcat(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.concat", args, error);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(error));
  const JSString* const str = val.ToString(args.ctx(), ERROR(error));
  core::UString result(str->begin(), str->end());
  for (Arguments::const_iterator it = args.begin(),
       last = args.end(); it != last; ++it) {
    const JSString* const r = it->ToString(args.ctx(), ERROR(error));
    result.append(r->begin(), r->end());
  }
  return JSString::New(args.ctx(), result);
}

// section 15.5.4.7 String.prototype.indexOf(searchString, position)
JSVal Runtime_StringIndexOf(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.indexOf", args, error);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(error));
  const JSString* const str = val.ToString(args.ctx(), ERROR(error));
  const JSString* search_str;
  // undefined -> NaN -> 0
  double position = 0;
  if (args.size() > 0) {
    search_str = args[0].ToString(args.ctx(), ERROR(error));
    if (args.size() > 1) {
      position = args[1].ToNumber(args.ctx(), ERROR(error));
      position = core::DoubleToInteger(position);
    }
  } else {
    // undefined -> "undefined"
    search_str = JSString::NewAsciiString(args.ctx(), "undefined");
  }
  const std::size_t start = std::min(
      static_cast<std::size_t>(std::max(position, 0.0)), str->size());
  const GCUString::size_type loc =
      str->value().find(search_str->value(), start);
  return (loc == GCUString::npos) ? -1.0 : loc;
}

// section 15.5.4.8 String.prototype.lastIndexOf(searchString, position)
JSVal Runtime_StringLastIndexOf(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.lastIndexOf", args, error);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(error));
  const JSString* const str = val.ToString(args.ctx(), ERROR(error));
  const JSString* search_str;
  // undefined -> NaN -> 0
  std::size_t pos = std::numeric_limits<std::size_t>::max();
  if (args.size() > 0) {
    search_str = args[0].ToString(args.ctx(), ERROR(error));
    if (args.size() > 1) {
      const double position = args[1].ToNumber(args.ctx(), ERROR(error));
      pos = core::DoubleToUInt32(core::DoubleToInteger(position));
    }
  } else {
    // undefined -> "undefined"
    search_str = JSString::NewAsciiString(args.ctx(), "undefined");
  }
  const GCUString::size_type loc =
      str->value().rfind(search_str->value(), std::min(pos, str->size()));
  return (loc == GCUString::npos) ? -1.0 : loc;
}

JSVal Runtime_BooleanConstructor(const Arguments& args, Error* error) {
  if (args.IsConstructorCalled()) {
    bool res = false;
    if (args.size() > 0) {
      res = args[0].ToBoolean(ERROR(error));
    }
    return JSBooleanObject::New(args.ctx(), res);
  } else {
    if (args.size() > 0) {
      const bool res = args[0].ToBoolean(ERROR(error));
      if (res) {
        return JSTrue;
      } else {
        return JSFalse;
      }
    } else {
      return JSFalse;
    }
  }
}

JSVal Runtime_BooleanToString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Boolean.prototype.toString", args, error);
  const JSVal& obj = args.this_binding();
  bool b;
  if (!obj.IsBoolean()) {
    if (obj.IsObject() && obj.object()->AsBooleanObject()) {
      b = obj.object()->AsBooleanObject()->value();
    } else {
      error->Report(Error::Type,
                    "Boolean.prototype.toString is not generic function");
      return JSUndefined;
    }
  } else {
    b = obj.boolean();
  }
  return JSString::NewAsciiString(args.ctx(), (b) ? "true" : "false");
}

JSVal Runtime_BooleanValueOf(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Boolean.prototype.valueOf", args, error);
  const JSVal& obj = args.this_binding();
  bool b;
  if (!obj.IsBoolean()) {
    if (obj.IsObject() && obj.object()->AsBooleanObject()) {
      b = obj.object()->AsBooleanObject()->value();
    } else {
      error->Report(Error::Type,
                    "Boolean.prototype.valueOf is not generic function");
      return JSUndefined;
    }
  } else {
    b = obj.boolean();
  }
  return JSVal::Bool(b);
}

JSVal Runtime_NumberConstructor(const Arguments& args, Error* error) {
  if (args.IsConstructorCalled()) {
    double res = 0.0;
    if (args.size() > 0) {
      res = args[0].ToNumber(args.ctx(), ERROR(error));
    }
    return JSNumberObject::New(args.ctx(), res);
  } else {
    if (args.size() > 0) {
      return args[0].ToNumber(args.ctx(), error);
    } else {
      return 0.0;
    }
  }
}

JSVal Runtime_NumberToString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Number.prototype.toString", args, error);
  const JSVal& obj = args.this_binding();
  double num;
  if (!obj.IsNumber()) {
    if (obj.IsObject() && obj.object()->AsNumberObject()) {
      num = obj.object()->AsNumberObject()->value();
    } else {
      error->Report(Error::Type,
                    "Number.prototype.toString is not generic function");
      return JSUndefined;
    }
  } else {
    num = obj.number();
  }
  if (args.size() > 0) {
    const JSVal& first = args[0];
    double radix = first.ToNumber(args.ctx(), ERROR(error));
    radix = core::DoubleToInteger(radix);
    if (2 <= radix && radix <= 36) {
      // if radix == 10, through to radix 10 or no radix
      if (radix != 10) {
        if (std::isnan(num)) {
          return JSString::NewAsciiString(args.ctx(), "NaN");
        } else if (!std::isfinite(num)) {
          if (num > 0) {
            return JSString::NewAsciiString(args.ctx(), "Infinity");
          } else {
            return JSString::NewAsciiString(args.ctx(), "-Infinity");
          }
        }
        JSString* const result = JSString::NewEmptyString(args.ctx());
        core::DoubleToStringWithRadix(
            num,
            static_cast<int>(radix),
            result);
        result->ReCalcHash();
        return result;
      }
    } else {
      // TODO(Constellation) more details
      error->Report(Error::Range,
                    "inllegal radix");
      return JSUndefined;
    }
  }
  // radix 10 or no radix
  std::tr1::array<char, 80> buffer;
  const char* const str = core::DoubleToCString(num,
                                                buffer.data(),
                                                buffer.size());
  return JSString::NewAsciiString(args.ctx(), str);
}

JSVal Runtime_NumberValueOf(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Number.prototype.valueOf", args, error);
  const JSVal& obj = args.this_binding();
  if (!obj.IsNumber()) {
    if (obj.IsObject() && obj.object()->AsNumberObject()) {
      return obj.object()->AsNumberObject()->value();
    } else {
      error->Report(Error::Type,
                    "Number.prototype.valueOf is not generic function");
      return JSUndefined;
    }
  } else {
    return obj.number();
  }
}

JSVal Runtime_MathAbs(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Math.abs", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    return std::abs(x);
  }
  return JSNaN;
}

JSVal Runtime_MathAcos(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Math.acos", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    return std::acos(x);
  }
  return JSNaN;
}

JSVal Runtime_MathAsin(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Math.asin", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    return std::asin(x);
  }
  return JSNaN;
}

JSVal Runtime_MathAtan(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Math.atan", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    return std::atan(x);
  }
  return JSNaN;
}

JSVal Runtime_MathAtan2(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Math.atan2", args, error);
  if (args.size() > 1) {
    const double y = args[0].ToNumber(args.ctx(), ERROR(error));
    const double x = args[1].ToNumber(args.ctx(), ERROR(error));
    return std::atan2(y, x);
  }
  return JSNaN;
}

JSVal Runtime_MathCeil(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Math.ceil", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    return std::ceil(x);
  }
  return JSNaN;
}

JSVal Runtime_MathCos(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Math.cos", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    return std::cos(x);
  }
  return JSNaN;
}

JSVal Runtime_MathExp(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Math.exp", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    return std::exp(x);
  }
  return JSNaN;
}

JSVal Runtime_MathFloor(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Math.floor", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    return std::floor(x);
  }
  return JSNaN;
}

JSVal Runtime_MathLog(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Math.log", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    return std::log(x);
  }
  return JSNaN;
}

JSVal Runtime_MathMax(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Math.max", args, error);
  double max = -kInfinity;
  for (Arguments::const_iterator it = args.begin(),
       last = args.end(); it != last; ++it) {
    const double x = it->ToNumber(args.ctx(), ERROR(error));
    if (std::isnan(x)) {
      return x;
    } else if (x > max) {
      max = x;
    }
  }
  return max;
}

JSVal Runtime_MathMin(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Math.min", args, error);
  double min = kInfinity;
  for (Arguments::const_iterator it = args.begin(),
       last = args.end(); it != last; ++it) {
    const double x = it->ToNumber(args.ctx(), ERROR(error));
    if (std::isnan(x)) {
      return x;
    } else if (x < min) {
      min = x;
    }
  }
  return min;
}

JSVal Runtime_MathPow(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Math.pow", args, error);
  if (args.size() > 1) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    const double y = args[1].ToNumber(args.ctx(), ERROR(error));
    return std::pow(x, y);
  }
  return JSNaN;
}

JSVal Runtime_MathRandom(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Math.random", args, error);
  return args.ctx()->Random();
}

JSVal Runtime_MathRound(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Math.round", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    const double res = std::ceil(x);
    if (res - x > 0.5) {
      return res - 1;
    } else {
      return res;
    }
  }
  return JSNaN;
}

JSVal Runtime_MathSin(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Math.sin", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    return std::sin(x);
  }
  return JSNaN;
}

JSVal Runtime_MathSqrt(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Math.sqrt", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    return std::sqrt(x);
  }
  return JSNaN;
}

JSVal Runtime_MathTan(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Math.tan", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    return std::tan(x);
  }
  return JSNaN;
}

} }  // namespace iv::lv5
