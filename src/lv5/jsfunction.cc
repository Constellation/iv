#include <iostream>  // NOLINT
#include "ast.h"
#include "lv5/lv5.h"
#include "lv5/jsfunction.h"
#include "lv5/jsenv.h"
#include "lv5/jsval.h"
#include "lv5/interpreter.h"
#include "lv5/context.h"
#include "lv5/arguments.h"
#include "lv5/jsscript.h"

namespace iv {
namespace lv5 {

void JSFunction::Initialize(Context* ctx) {
  // section 13.2 Creating Function Objects
  const Class& cls = ctx->Cls("Function");
  set_class_name(cls.name);
  set_prototype(cls.prototype);

  JSObject* const proto = JSObject::New(ctx);
  proto->DefineOwnProperty(
      ctx, ctx->constructor_symbol(),
      DataDescriptor(this,
                     PropertyDescriptor::WRITABLE |
                     PropertyDescriptor::CONFIGURABLE),
                     false, NULL);
  DefineOwnProperty(
      ctx, ctx->prototype_symbol(),
      DataDescriptor(proto,
                     PropertyDescriptor::WRITABLE),
                     false, NULL);
  if (ctx->IsStrict()) {
    JSFunction* const throw_type_error = ctx->throw_type_error();
    DefineOwnProperty(ctx, ctx->caller_symbol(),
                      AccessorDescriptor(throw_type_error,
                                         throw_type_error,
                                         PropertyDescriptor::NONE),
                      false, NULL);
    DefineOwnProperty(ctx, ctx->arguments_symbol(),
                      AccessorDescriptor(throw_type_error,
                                         throw_type_error,
                                         PropertyDescriptor::NONE),
                      false, NULL);
  }
}

JSCodeFunction::JSCodeFunction(Context* ctx,
                               const FunctionLiteral* func,
                               JSScript* script,
                               JSEnv* env)
  : function_(func),
    script_(script),
    env_(env) {
  DefineOwnProperty(
      ctx, ctx->length_symbol(),
      DataDescriptor(func->params().size(),
                     PropertyDescriptor::NONE),
                     false, NULL);
}

JSVal JSCodeFunction::Call(Arguments& args,
                           const JSVal& this_binding, Error* error) {
  Interpreter* const interp = args.ctx()->interp();
  Context* const ctx = args.ctx();
  args.set_this_binding(this_binding);
  interp->CallCode(this, args, error);
  if (ctx->mode() == Context::RETURN) {
    ctx->set_mode(Context::NORMAL);
  }
  assert(!ctx->ret().IsEmpty() || *error);
  return ctx->ret();
}

JSVal JSCodeFunction::Construct(Arguments& args, Error* e) {
  Context* const ctx = args.ctx();
  JSObject* const obj = JSObject::New(ctx);
  const JSVal proto = Get(ctx, ctx->prototype_symbol(), ERROR(e));
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

bool JSFunction::HasInstance(Context* ctx,
                             const JSVal& val, Error* error) {
  if (!val.IsObject()) {
    return false;
  }
  const JSVal got = Get(ctx, ctx->prototype_symbol(), error);
  if (*error) {
    return false;
  }
  if (!got.IsObject()) {
    error->Report(Error::Type, "\"prototype\" is not object");
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

JSVal JSFunction::Get(Context* ctx,
                      Symbol name, Error* error) {
  const JSVal val = JSObject::Get(ctx, name, error);
  if (*error) {
    return val;
  }
  if (name == ctx->caller_symbol() &&
      val.IsCallable() &&
      val.object()->AsCallable()->IsStrict()) {
    error->Report(Error::Type,
                  "\"caller\" property is not accessible in strict code");
    return JSFalse;
  }
  return val;
}

void JSNativeFunction::Initialize(Context* ctx,
                                  value_type func, std::size_t n) {
  func_ = func;
  DefineOwnProperty(
      ctx, ctx->length_symbol(),
      DataDescriptor(n,
                     PropertyDescriptor::NONE),
                     false, NULL);
  InitializeSimple(ctx);
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
  const Class& cls = ctx->Cls("Function");
  set_class_name(cls.name);
  set_prototype(cls.prototype);
  // step 15
  if (target_->class_name() == cls.name) {
    // target [[Class]] is "Function"
    const JSVal length = target_->Get(ctx, ctx->length_symbol(), ctx->error());
    assert(length.IsNumber());
    const uint32_t target_param_size = core::DoubleToUInt32(length.number());
    assert(target_param_size == length.number());
    const uint32_t len = (target_param_size >= bound_args_size) ?
        target_param_size - bound_args_size : 0;
    DefineOwnProperty(
        ctx, ctx->length_symbol(),
        DataDescriptor(len,
                       PropertyDescriptor::NONE),
                       false, NULL);
  } else {
    DefineOwnProperty(
        ctx, ctx->length_symbol(),
        DataDescriptor(0.0,
                       PropertyDescriptor::NONE),
                       false, NULL);
  }
  JSFunction* const throw_type_error = ctx->throw_type_error();
  DefineOwnProperty(ctx, ctx->caller_symbol(),
                    AccessorDescriptor(throw_type_error,
                                       throw_type_error,
                                       PropertyDescriptor::NONE),
                    false, ctx->error());
  DefineOwnProperty(ctx, ctx->callee_symbol(),
                    AccessorDescriptor(throw_type_error,
                                       throw_type_error,
                                       PropertyDescriptor::NONE),
                    false, ctx->error());
}

JSVal JSBoundFunction::Call(Arguments& args,
                            const JSVal& this_binding, Error* e) {
  using std::copy;
  Arguments args_list(args.ctx(), args.size() + arguments_.size(), ERROR(e));
  copy(args.begin(), args.end(),
       copy(arguments_.begin(), arguments_.end(), args_list.begin()));
  return target_->Call(args_list, this_binding_, e);
}

JSVal JSBoundFunction::Construct(Arguments& args, Error* e) {
  using std::copy;
  Arguments args_list(args.ctx(), args.size() + arguments_.size(), ERROR(e));
  copy(args.begin(), args.end(),
       copy(arguments_.begin(), arguments_.end(), args_list.begin()));
  return target_->Construct(args_list, e);
}

bool JSBoundFunction::HasInstance(Context* ctx,
                                  const JSVal& val, Error* e) {
  return target_->HasInstance(ctx, val, e);
}

JSBoundFunction* JSBoundFunction::New(Context* ctx,
                                      JSFunction* target,
                                      const JSVal& this_binding,
                                      const Arguments& args) {
  JSBoundFunction* const bound =
      new JSBoundFunction(ctx, target, this_binding, args);
  return bound;
}

} }  // namespace iv::lv5
