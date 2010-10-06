#include "cstring"
#include "ustring.h"
#include "arguments.h"
#include "context.h"
#include "interpreter.h"
#include "jsproperty.h"
#include "class.h"
#include "runtime.h"
namespace iv {
namespace lv5 {
namespace {
const char * const function_prefix = "function ";

JSVal ObjectConstructor(const Arguments& args, JSErrorCode::Type* error) {
  if (args.size() == 1) {
    const JSVal& val = args[0];
    if (val.IsNull() || val.IsUndefined()) {
      return JSVal(JSObject::New(args.context()));
    } else {
      JSObject* const obj = val.ToObject(args.context(), error);
      if (*error) {
        return JSVal::Undefined();
      }
      return JSVal(obj);
    }
  } else {
    return JSVal(JSObject::New(args.context()));
  }
}

JSVal ObjectHasOwnProperty(const Arguments& args, JSErrorCode::Type* error) {
  if (args.size() > 0) {
    const JSVal& val = args[0];
    Context* ctx = args.context();
    JSString* const str = val.ToString(ctx, error);
    JSObject* const obj = args.this_binding().ToObject(ctx, error);
    if (*error) {
      return JSVal(false);
    }
    return JSVal(!!obj->GetOwnProperty(ctx->Intern(str->data())));
  } else {
    return JSVal(false);
  }
}

JSVal ObjectToString(const Arguments& args, JSErrorCode::Type* error) {
  std::string ascii;
  JSObject* const obj = args.this_binding().ToObject(args.context(), error);
  if (*error) {
    return JSVal(false);
  }
  JSString* const cls = obj->cls();
  assert(cls);
  std::string str("[object ");
  str.append(cls->begin(), cls->end());
  str.append("]");
  return JSVal(JSString::NewAsciiString(args.context(), str.c_str()));
}

JSVal FunctionToString(const Arguments& args, JSErrorCode::Type* error) {
  const JSVal& obj = args.this_binding();
  if (obj.IsObject() && obj.object()->IsCallable()) {
    JSFunction* const func = obj.object()->AsCallable();
    if (func->AsNativeFunction()) {
      return JSVal(JSString::NewAsciiString(args.context(), "function () { [native code] }"));
    } else {
      core::UString buffer(function_prefix,
                           function_prefix+std::strlen(function_prefix));
      if (func->AsCodeFunction()->name()) {
        const core::UStringPiece name = func->AsCodeFunction()->name()->value();
        buffer.append(name.data(), name.size());
      }
      const core::UStringPiece src = func->AsCodeFunction()->GetSource();
      buffer.append(src.data(), src.size());
      return JSVal(JSString::New(args.context(), buffer));
    }
  }
  *error = JSErrorCode::TypeError;
  return JSVal::Undefined();
}

}  // namespace

Context::Context(Interpreter* interp)
  : global_obj_(),
    lexical_env_(NULL),
    variable_env_(NULL),
    binding_(&global_obj_),
    table_(),
    interp_(interp),
    mode_(NORMAL),
    ret_(),
    target_(NULL),
    error_(JSErrorCode::Normal),
    builtins_(),
    strict_(false) {
  JSEnv* env = Interpreter::NewObjectEnvironment(this, &global_obj_, NULL);
  lexical_env_ = env;
  variable_env_ = env;
  interp_->set_context(this);
  Prelude();
}

Symbol Context::Intern(core::StringPiece str) {
  return table_.Lookup(str.data(), str.size());
}

Symbol Context::Intern(core::UStringPiece str) {
  return table_.Lookup(str.data(), str.size());
}

JSString* Context::ToString(Symbol sym) {
  return table_.ToString(this, sym);
}

bool Context::InCurrentLabelSet(
    const core::AnonymousBreakableStatement* stmt) const {
  // AnonymousBreakableStatement has empty label at first
  return !target_ || stmt == target_;
}

bool Context::InCurrentLabelSet(
    const core::NamedOnlyBreakableStatement* stmt) const {
  return stmt == target_;
}

void Context::Prelude() {
  // Object
  JSNativeFunction* const obj_constructor =
      JSNativeFunction::New(this, &ObjectConstructor);
  JSObject* const obj_proto = JSObject::NewPlain(this);
  obj_proto->set_cls(JSString::NewAsciiString(this, "Object"));

  struct Class obj_cls = {
    JSString::NewAsciiString(this, "Object"),
    obj_constructor,
    obj_proto
  };
  obj_proto->set_cls(obj_cls.name);

  // Function
  JSObject* const func_proto = JSObject::NewPlain(this);
  func_proto->set_prototype(obj_proto);
  struct Class func_cls = {
    JSString::NewAsciiString(this, "Function"),
    NULL,
    func_proto
  };
  func_proto->set_cls(func_cls.name);
  const Symbol func_name = Intern("Function");
  builtins_[func_name] = func_cls;
  {
    JSNativeFunction* const func =
        JSNativeFunction::New(this, &FunctionToString);
    func_proto->DefineOwnProperty(
        this, Intern("toString"),
        new DataDescriptor(JSVal(func),
                           PropertyDescriptor::WRITABLE |
                           PropertyDescriptor::ENUMERABLE),
        false, NULL);
  }

  {
    // Object Define
    const Symbol name = Intern("Object");
    {
      JSNativeFunction* const func =
          JSNativeFunction::New(this, &ObjectHasOwnProperty);
      obj_proto->DefineOwnProperty(
          this, Intern("hasOwnProperty"),
          new DataDescriptor(JSVal(func),
                             PropertyDescriptor::WRITABLE |
                             PropertyDescriptor::ENUMERABLE),
          false, NULL);
    }
    {
      JSNativeFunction* const func =
          JSNativeFunction::New(this, &ObjectToString);
      obj_proto->DefineOwnProperty(
          this, Intern("toString"),
          new DataDescriptor(JSVal(func),
                             PropertyDescriptor::WRITABLE |
                             PropertyDescriptor::ENUMERABLE),
          false, NULL);
    }
    builtins_[name] = obj_cls;

    obj_constructor->DefineOwnProperty(
        this, Intern("prototype"),
        new DataDescriptor(JSVal(obj_proto), PropertyDescriptor::NONE),
        false, NULL);

    variable_env_->CreateMutableBinding(this, name, false);
    variable_env_->SetMutableBinding(this, name,
                                     JSVal(obj_constructor), strict_, NULL);
  }
  {
    // Array
    JSObject* const proto = JSObject::NewPlain(this);
    proto->set_prototype(obj_proto);
    struct Class cls = {
      JSString::NewAsciiString(this, "Array"),
      NULL,
      proto
    };
    proto->set_cls(cls.name);
    const Symbol name = Intern("Array");
    builtins_[name] = cls;
  }
  global_obj_.set_cls(JSString::NewAsciiString(this, "global"));
  global_obj_.set_prototype(obj_proto);
}

const Class& Context::Cls(Symbol name) {
  return builtins_[name];
}

const Class& Context::Cls(core::StringPiece str) {
  return builtins_[Intern(str)];
}

} }  // namespace iv::lv5
