#include "runtime.h"
#include "context.h"
namespace iv {
namespace lv5 {
namespace {

const std::string function_prefix("function");

}  // namespace

JSVal Runtime_ObjectConstructor(const Arguments& args, JSErrorCode::Type* error) {
  if (args.size() == 1) {
    const JSVal& val = args[0];
    if (val.IsNull() || val.IsUndefined()) {
      return JSVal(JSObject::New(args.ctx()));
    } else {
      JSObject* const obj = val.ToObject(args.ctx(), error);
      if (*error) {
        return JSVal::Undefined();
      }
      return JSVal(obj);
    }
  } else {
    return JSVal(JSObject::New(args.ctx()));
  }
}

JSVal Runtime_ObjectHasOwnProperty(const Arguments& args, JSErrorCode::Type* error) {
  if (args.size() > 0) {
    const JSVal& val = args[0];
    Context* ctx = args.ctx();
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

JSVal Runtime_ObjectToString(const Arguments& args, JSErrorCode::Type* error) {
  std::string ascii;
  JSObject* const obj = args.this_binding().ToObject(args.ctx(), error);
  if (*error) {
    return JSVal(false);
  }
  JSString* const cls = obj->cls();
  assert(cls);
  std::string str("[object ");
  str.append(cls->begin(), cls->end());
  str.append("]");
  return JSVal(JSString::NewAsciiString(args.ctx(), str.c_str()));
}

JSVal Runtime_FunctionToString(const Arguments& args, JSErrorCode::Type* error) {
  const JSVal& obj = args.this_binding();
  if (obj.IsCallable()) {
    JSFunction* const func = obj.object()->AsCallable();
    if (func->AsNativeFunction()) {
      return JSVal(JSString::NewAsciiString(args.ctx(), "function () { [native code] }"));
    } else {
      core::UString buffer(function_prefix.begin(),
                           function_prefix.end());
      if (func->AsCodeFunction()->name()) {
        const core::UStringPiece name = func->AsCodeFunction()->name()->value();
        buffer.append(name.data(), name.size());
      }
      const core::UStringPiece src = func->AsCodeFunction()->GetSource();
      buffer.append(src.data(), src.size());
      return JSVal(JSString::New(args.ctx(), buffer));
    }
  }
  *error = JSErrorCode::TypeError;
  return JSVal::Undefined();
}

} }  // namespace iv::lv5
