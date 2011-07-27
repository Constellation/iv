#ifndef _IV_LV5_TELEPORTER_JSFUNCTION_H_
#define _IV_LV5_TELEPORTER_JSFUNCTION_H_
#include "ustringpiece.h"
#include "lv5/context_utils.h"
#include "lv5/jsobject.h"
#include "lv5/error.h"
#include "lv5/class.h"
#include "lv5/arguments.h"
#include "lv5/specialized_ast.h"
#include "lv5/error_check.h"
#include "lv5/jsfunction.h"
#include "lv5/teleporter/fwd.h"
#include "lv5/teleporter/jsscript.h"
#include "lv5/teleporter/context.h"
#include "lv5/teleporter/interpreter_fwd.h"
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
        ctx, symbol::length(),
        DataDescriptor(
            JSVal::UInt32(static_cast<uint32_t>(func->params().size())),
            PropertyDescriptor::NONE),
        false, &e);
    // section 13.2 Creating Function Objects
    set_cls(JSFunction::GetClass());
    set_prototype(context::GetClassSlot(ctx, Class::Function).prototype);

    JSObject* const proto = JSObject::New(ctx);
    proto->DefineOwnProperty(
        ctx, symbol::constructor(),
        DataDescriptor(this,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, &e);
    DefineOwnProperty(
        ctx, symbol::prototype(),
        DataDescriptor(proto,
                       PropertyDescriptor::WRITABLE),
        false, &e);
    if (const core::Maybe<const Identifier> ident = function_->name()) {
      const core::UStringPiece name = ident.Address()->value();
      if (!name.empty()) {
        DefineOwnProperty(
            ctx, context::Intern(ctx, "name"),
            DataDescriptor(JSString::New(ctx, name),
                           PropertyDescriptor::NONE),
            false, &e);
      } else {
        DefineOwnProperty(
            ctx, context::Intern(ctx, "name"),
            DataDescriptor(
                JSString::NewEmptyString(ctx),
                PropertyDescriptor::NONE),
            false, &e);
      }
    }
    if (function_->strict()) {
      JSFunction* const throw_type_error = ctx->throw_type_error();
      DefineOwnProperty(ctx, symbol::caller(),
                        AccessorDescriptor(throw_type_error,
                                           throw_type_error,
                                           PropertyDescriptor::NONE),
                        false, &e);
      DefineOwnProperty(ctx, symbol::arguments(),
                        AccessorDescriptor(throw_type_error,
                                           throw_type_error,
                                           PropertyDescriptor::NONE),
                        false, &e);
    }
  }

  JSVal Call(Arguments* args,
             const JSVal& this_binding, Error* e) {
    Context* const ctx = static_cast<Context*>(args->ctx());
    args->set_this_binding(this_binding);
    ctx->interp()->Invoke(this, *args, e);
    if (ctx->mode() == Context::RETURN) {
      ctx->set_mode(Context::NORMAL);
    }
    assert(!ctx->ret().IsEmpty() || *e);
    return ctx->ret();
  }

  JSVal Construct(Arguments* args, Error* e) {
    iv::lv5::Context* ctx = args->ctx();
    JSObject* const obj = JSObject::New(ctx);
    const JSVal proto = Get(ctx, symbol::prototype(), IV_LV5_ERROR(e));
    if (proto.IsObject()) {
      obj->set_prototype(proto.object());
    }
    const JSVal result = Call(args, obj, IV_LV5_ERROR(e));
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

  bool IsBoundFunction() const {
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
