#ifndef _IV_LV5_TELEPORTER_JSFUNCTION_H_
#define _IV_LV5_TELEPORTER_JSFUNCTION_H_
#include "ustringpiece.h"
#include "lv5/context_utils.h"
#include "lv5/jsobject.h"
#include "lv5/error.h"
#include "lv5/arguments.h"
#include "lv5/jsast.h"
#include "lv5/lv5.h"
#include "lv5/jsfunction.h"
#include "lv5/teleporter_fwd.h"
#include "lv5/teleporter_jsscript.h"
#include "lv5/teleporter_context.h"
namespace iv {
namespace lv5 {

class JSEnv;
class JSScript;

namespace teleporter {

class JSCodeFunction : public JSFunction {
 public:
  JSCodeFunction(Context* ctx,
                 const FunctionLiteral* func,
                 JSScript* script,
                 JSEnv* env)
    : function_(func),
      script_(script),
      env_(env) {
    Error e;
    DefineOwnProperty(
        ctx, context::length_symbol(ctx),
        DataDescriptor(
            JSVal::UInt32(static_cast<uint32_t>(func->params().size())),
            PropertyDescriptor::NONE),
        false, &e);
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
        false, &e);
    DefineOwnProperty(
        ctx, context::prototype_symbol(ctx),
        DataDescriptor(proto,
                       PropertyDescriptor::WRITABLE),
        false, &e);
    core::UStringPiece name = GetName();
    if (!name.empty()) {
      DefineOwnProperty(
          ctx, context::Intern(ctx, "name"),
          DataDescriptor(JSString::New(ctx, name),
                         PropertyDescriptor::NONE),
          false, &e);
    }
    if (ctx->IsStrict()) {
      JSFunction* const throw_type_error = ctx->throw_type_error();
      DefineOwnProperty(ctx, context::caller_symbol(ctx),
                        AccessorDescriptor(throw_type_error,
                                           throw_type_error,
                                           PropertyDescriptor::NONE),
                        false, &e);
      DefineOwnProperty(ctx, context::arguments_symbol(ctx),
                        AccessorDescriptor(throw_type_error,
                                           throw_type_error,
                                           PropertyDescriptor::NONE),
                        false, &e);
    }
  }

  JSVal Call(Arguments* args, const JSVal& this_binding, Error* e) {
    Context* const ctx = static_cast<Context*>(args->ctx());
    Interpreter* const interp = ctx->interp();
    args->set_this_binding(this_binding);
    interp->CallCode(this, *args, e);
    if (ctx->mode() == Context::RETURN) {
      ctx->set_mode(Context::NORMAL);
    }
    assert(!ctx->ret().IsEmpty() || *e);
    return ctx->ret();
  }

  JSVal Construct(Arguments* args, Error* e) {
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

  JSEnv* scope() const {
    return env_;
  }

  const FunctionLiteral* code() const {
    return function_;
  }

  static JSCodeFunction* New(Context* ctx,
                             const FunctionLiteral* func,
                             JSScript* script,
                             JSEnv* env) {
    return new JSCodeFunction(ctx, func, script, env);
  }

  bool IsNativeFunction() const {
    return false;
  }

  core::UStringPiece GetSource() const {
    const std::size_t start_pos = function_->start_position();
    const std::size_t end_pos = function_->end_position();
    return script_->SubString(start_pos, end_pos - start_pos);
  }

  core::UStringPiece GetName() const {
    if (const core::Maybe<const Identifier> name = function_->name()) {
      return name.Address()->value();
    } else {
      return core::UStringPiece();
    }
  }

  core::Maybe<const Identifier> name() const {
    return function_->name();
  }

  bool IsStrict() const {
    return function_->strict();
  }

 private:
  const FunctionLiteral* function_;
  JSScript* script_;
  JSEnv* env_;
};

} } }  // namespace iv::lv5::teleporter
#endif  // _IV_LV5_TELEPORTER_JSFUNCTION_H_
