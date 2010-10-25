#include <iostream>  // NOLINT
#include "lv5.h"
#include "error.h"
#include "runtime.h"
#include "context.h"
namespace iv {
namespace lv5 {
namespace {

const std::string function_prefix("function ");
const std::string error_split(": ");

}  // namespace

JSVal Runtime_ObjectConstructor(const Arguments& args,
                                Error* error) {
  if (args.size() == 1) {
    const JSVal& val = args[0];
    if (val.IsNull() || val.IsUndefined()) {
      return JSObject::New(args.ctx());
    } else {
      JSObject* const obj = val.ToObject(args.ctx(), ERROR(error));
      return obj;
    }
  } else {
    return JSObject::New(args.ctx());
  }
}

JSVal Runtime_ObjectHasOwnProperty(const Arguments& args,
                                   Error* error) {
  if (args.size() > 0) {
    const JSVal& val = args[0];
    Context* ctx = args.ctx();
    JSString* const str = val.ToString(ctx, ERROR(error));
    JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(error));
    if (!obj->GetOwnProperty(ctx->Intern(*str)).IsEmpty()) {
      return JSTrue;
    } else {
      return JSFalse;
    }
  } else {
    return JSFalse;
  }
}

JSVal Runtime_ObjectToString(const Arguments& args, Error* error) {
  std::string ascii;
  JSObject* const obj = args.this_binding().ToObject(args.ctx(), ERROR(error));
  JSString* const cls = obj->cls();
  assert(cls);
  std::string str("[object ");
  str.append(cls->begin(), cls->end());
  str.append("]");
  return JSString::NewAsciiString(args.ctx(), str.c_str());
}

JSVal Runtime_FunctionToString(const Arguments& args,
                               Error* error) {
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

JSVal Runtime_ErrorToString(const Arguments& args, Error* error) {
  const JSVal& obj = args.this_binding();
  Context* ctx = args.ctx();
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

JSVal Runtime_MathRandom(const Arguments& args, Error* error) {
  return args.ctx()->Random();
}

} }  // namespace iv::lv5
