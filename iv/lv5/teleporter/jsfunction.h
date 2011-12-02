#ifndef IV_LV5_TELEPORTER_JSFUNCTION_H_
#define IV_LV5_TELEPORTER_JSFUNCTION_H_
#include <iv/ustringpiece.h>
#include <iv/lv5/context_utils.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/error.h>
#include <iv/lv5/class.h>
#include <iv/lv5/map.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/specialized_ast.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/jsfunction.h>
#include <iv/lv5/teleporter/fwd.h>
#include <iv/lv5/teleporter/jsscript.h>
#include <iv/lv5/teleporter/context.h>
#include <iv/lv5/teleporter/interpreter_fwd.h>
namespace iv {
namespace lv5 {

class JSEnv;
class JSScript;

namespace teleporter {

class JSCodeFunction : public JSFunction {
 public:
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

  core::UString GetName() const {
    if (const core::Maybe<const Identifier> name = function_->name()) {
      return symbol::GetSymbolString(name.Address()->symbol());
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

  void MarkChildren(radio::Core* core) {
    core->MarkCell(script_);
    core->MarkCell(env_);
  }

 private:
  JSCodeFunction(Context* ctx,
                 const FunctionLiteral* func,
                 JSScript* script,
                 JSEnv* env)
    : JSFunction(ctx),
      function_(func),
      script_(script),
      env_(env) {
    Error e;
    DefineOwnProperty(
        ctx, symbol::length(),
        DataDescriptor(
            JSVal::UInt32(static_cast<uint32_t>(func->params().size())),
            ATTR::NONE),
        false, &e);
    // section 13.2 Creating Function Objects
    set_cls(JSFunction::GetClass());
    set_prototype(context::GetClassSlot(ctx, Class::Function).prototype);

    JSObject* const proto = JSObject::New(ctx);
    proto->DefineOwnProperty(
        ctx, symbol::constructor(),
        DataDescriptor(this,
                       ATTR::WRITABLE |
                       ATTR::CONFIGURABLE),
        false, &e);
    DefineOwnProperty(
        ctx, symbol::prototype(),
        DataDescriptor(proto,
                       ATTR::WRITABLE),
        false, &e);
    if (const core::Maybe<const Identifier> ident = function_->name()) {
      const core::UString name =
          symbol::GetSymbolString(ident.Address()->symbol());
      if (!name.empty()) {
        DefineOwnProperty(
            ctx, context::Intern(ctx, "name"),
            DataDescriptor(JSString::New(ctx, name),
                           ATTR::NONE),
            false, &e);
      } else {
        DefineOwnProperty(
            ctx, context::Intern(ctx, "name"),
            DataDescriptor(
                JSString::NewEmptyString(ctx),
                ATTR::NONE),
            false, &e);
      }
    }
    if (function_->strict()) {
      JSFunction* const throw_type_error = ctx->throw_type_error();
      DefineOwnProperty(ctx, symbol::caller(),
                        AccessorDescriptor(throw_type_error,
                                           throw_type_error,
                                           ATTR::NONE),
                        false, &e);
      DefineOwnProperty(ctx, symbol::arguments(),
                        AccessorDescriptor(throw_type_error,
                                           throw_type_error,
                                           ATTR::NONE),
                        false, &e);
    }
  }


  const FunctionLiteral* function_;
  JSScript* script_;
  JSEnv* env_;
};

} } }  // namespace iv::lv5::teleporter
#endif  // IV_LV5_TELEPORTER_JSFUNCTION_H_
