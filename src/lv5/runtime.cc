#include <iostream>
#include "lv5.h"
#include "runtime.h"
#include "context.h"
namespace iv {
namespace lv5 {
namespace {

const std::string function_prefix("function ");

}  // namespace

JSVal Runtime_ObjectConstructor(const Arguments& args, JSErrorCode::Type* error) {
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

JSVal Runtime_ObjectHasOwnProperty(const Arguments& args, JSErrorCode::Type* error) {
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

JSVal Runtime_ObjectToString(const Arguments& args, JSErrorCode::Type* error) {
  std::string ascii;
  JSObject* const obj = args.this_binding().ToObject(args.ctx(), ERROR(error));
  JSString* const cls = obj->cls();
  assert(cls);
  std::string str("[object ");
  str.append(cls->begin(), cls->end());
  str.append("]");
  return JSString::NewAsciiString(args.ctx(), str.c_str());
}

JSVal Runtime_FunctionToString(const Arguments& args, JSErrorCode::Type* error) {
  const JSVal& obj = args.this_binding();
  if (obj.IsCallable()) {
    JSFunction* const func = obj.object()->AsCallable();
    if (func->AsNativeFunction()) {
      return JSString::NewAsciiString(args.ctx(), "function () { [native code] }");
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
  *error = JSErrorCode::TypeError;
  return JSUndefined;
}

} }  // namespace iv::lv5
