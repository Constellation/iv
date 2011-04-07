#include <iostream>  // NOLINT
#include "ast.h"
#include "lv5/lv5.h"
#include "lv5/jsfunction.h"
#include "lv5/jsenv.h"
#include "lv5/jsval.h"
#include "lv5/interpreter.h"
#include "lv5/context.h"
#include "lv5/context_utils.h"
#include "lv5/arguments.h"
#include "lv5/jsscript.h"

namespace iv {
namespace lv5 {

JSCodeFunction::JSCodeFunction(Context* ctx,
                               const FunctionLiteral* func,
                               JSScript* script,
                               JSEnv* env)
  : function_(func),
    script_(script),
    env_(env) {
  DefineOwnProperty(
      ctx, context::length_symbol(ctx),
      DataDescriptor(func->params().size(),
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
  Context* const ctx = args->ctx();
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
  Context* const ctx = args->ctx();
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

JSBoundFunction::JSBoundFunction(Context* ctx,
                                 JSFunction* target,
                                 const JSVal& this_binding,
                                 const Arguments& args)
  : target_(target),
    this_binding_(this_binding),
    arguments_(args.size() == 0 ? 0 : args.size() - 1) {
  using std::copy;
  if (args.size() > 0) {
    copy(args.begin() + 1, args.end(), arguments_.begin());
  }
  const uint32_t bound_args_size = arguments_.size();
  const Class& cls = context::Cls(ctx, "Function");
  set_class_name(cls.name);
  set_prototype(cls.prototype);
  // step 15
  if (target_->class_name() == cls.name) {
    // target [[Class]] is "Function"
    const JSVal length = target_->Get(ctx, context::length_symbol(ctx), ctx->error());
    assert(length.IsNumber());
    const uint32_t target_param_size = core::DoubleToUInt32(length.number());
    assert(target_param_size == length.number());
    const uint32_t len = (target_param_size >= bound_args_size) ?
        target_param_size - bound_args_size : 0;
    DefineOwnProperty(
        ctx, context::length_symbol(ctx),
        DataDescriptor(len,
                       PropertyDescriptor::NONE),
                       false, NULL);
  } else {
    DefineOwnProperty(
        ctx, context::length_symbol(ctx),
        DataDescriptor(0.0,
                       PropertyDescriptor::NONE),
                       false, NULL);
  }
  JSFunction* const throw_type_error = ctx->throw_type_error();
  DefineOwnProperty(ctx, context::caller_symbol(ctx),
                    AccessorDescriptor(throw_type_error,
                                       throw_type_error,
                                       PropertyDescriptor::NONE),
                    false, ctx->error());
  DefineOwnProperty(ctx, context::arguments_symbol(ctx),
                    AccessorDescriptor(throw_type_error,
                                       throw_type_error,
                                       PropertyDescriptor::NONE),
                    false, ctx->error());
}

} }  // namespace iv::lv5
