#include "ast.h"
#include "jsenv.h"
#include "jsfunction.h"
#include "jsval.h"
#include "jsobject.h"
#include "interpreter.h"
#include "context.h"
#include "arguments.h"
namespace iv {
namespace lv5 {

void JSFunction::SetClass(Context* ctx, JSObject* obj) {
  const Class& cls = ctx->Cls("Function");
  obj->set_cls(cls.name);
  obj->set_prototype(cls.prototype);
}

JSCodeFunction::JSCodeFunction(core::FunctionLiteral* func,
                               JSEnv* env)
  : function_(func),
    env_(env) {
}

JSVal JSCodeFunction::Call(const Arguments& args,
                           JSErrorCode::Type* error) {
  Interpreter* const interp = args.interpreter();
  Context* const context = args.context();
  interp->CallCode(*this, args, error);
  if (context->mode() == Context::RETURN) {
    context->set_mode(Context::NORMAL);
  }
  return context->ret();
}

JSCodeFunction* JSCodeFunction::New(Context* ctx,
                                    core::FunctionLiteral* func, JSEnv* env) {
  JSCodeFunction* const obj = new JSCodeFunction(func, env);
  SetClass(ctx, obj);
  return obj;
}

bool JSFunction::HasInstance(Context* context,
                             const JSVal& val, JSErrorCode::Type* error) {
  if (!val.IsObject()) {
    return false;
  }
  const JSVal got = Get(context, context->Intern("prototype"), error);
  if (*error) {
    return false;
  }
  if (!got.IsObject()) {
    *error = JSErrorCode::TypeError;
    return false;
  }
  const JSObject* const proto = got.object();
  const JSObject* obj = val.object()->prototype();
  while (obj) {
    if (obj == proto) {
      return true;
    } else {
      obj = obj->prototype();
    }
  }
  return false;
}

JSVal JSNativeFunction::Call(const Arguments& args,
                             JSErrorCode::Type* error) {
  return func_(args, error);
}

} }  // namespace iv::lv5
