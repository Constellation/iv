#include <iostream>  // NOLINT
#include <algorithm>
#include "ast.h"
#include "lv5/lv5.h"
#include "lv5/jsfunction.h"
#include "lv5/jsenv.h"
#include "lv5/jsval.h"
#include "lv5/context.h"
#include "lv5/context_utils.h"
#include "lv5/arguments.h"
#include "lv5/bind.h"
#include "lv5/teleporter.h"

namespace iv {
namespace lv5 {
namespace teleporter {

JSCodeFunction::JSCodeFunction(Context* ctx,
                               const FunctionLiteral* func,
                               JSScript* script,
                               JSEnv* env)
  : function_(func),
    script_(script),
    env_(env) {
  DefineOwnProperty(
      ctx, context::length_symbol(ctx),
      DataDescriptor(JSVal::UInt32(func->params().size()),
                     PropertyDescriptor::NONE),
                     false, NULL);
  // section 13.2 Creating Function Objects
  const Class& cls = context::Cls(ctx, "Function");
  set_class_name(cls.name);
  set_prototype(cls.prototype);

  JSObject* const proto = JSObject::New(ctx);
  proto->DefineOwnProperty(
      ctx, context::constructor_symbol(ctx),
      DataDescriptor(this,
                     PropertyDescriptor::WRITABLE |
                     PropertyDescriptor::CONFIGURABLE),
                     false, NULL);
  DefineOwnProperty(
      ctx, context::prototype_symbol(ctx),
      DataDescriptor(proto,
                     PropertyDescriptor::WRITABLE),
                     false, NULL);
  if (ctx->IsStrict()) {
    JSFunction* const throw_type_error = ctx->throw_type_error();
    DefineOwnProperty(ctx, context::caller_symbol(ctx),
                      AccessorDescriptor(throw_type_error,
                                         throw_type_error,
                                         PropertyDescriptor::NONE),
                      false, NULL);
    DefineOwnProperty(ctx, context::arguments_symbol(ctx),
                      AccessorDescriptor(throw_type_error,
                                         throw_type_error,
                                         PropertyDescriptor::NONE),
                      false, NULL);
  }
}

JSVal JSCodeFunction::Call(Arguments* args,
                           const JSVal& this_binding, Error* error) {
  Context* const ctx = static_cast<Context*>(args->ctx());
  Interpreter* const interp = ctx->interp();
  args->set_this_binding(this_binding);
  interp->CallCode(this, *args, error);
  if (ctx->mode() == Context::RETURN) {
    ctx->set_mode(Context::NORMAL);
  }
  assert(!ctx->ret().IsEmpty() || *error);
  return ctx->ret();
}

JSVal JSCodeFunction::Construct(Arguments* args, Error* e) {
  Context* const ctx = static_cast<Context*>(args->ctx());
  JSObject* const obj = JSObject::New(ctx);
  const JSVal proto = Get(ctx, context::prototype_symbol(ctx), ERROR(e));
  if (proto.IsObject()) {
    obj->set_prototype(proto.object());
  }
  const JSVal result = Call(args, obj, ERROR(e));
  if (result.IsObject()) {
    return result;
  } else {
    return obj;
  }
}

core::UStringPiece JSCodeFunction::GetSource() const {
  const std::size_t start_pos = function_->start_position();
  const std::size_t end_pos = function_->end_position();
  return script_->SubString(start_pos,
                            end_pos - start_pos);
}

} } }  // namespace iv::lv5::teleporter
