#ifndef IV_LV5_RAILGUN_JSFUNCTION_H_
#define IV_LV5_RAILGUN_JSFUNCTION_H_
#include <iv/ustringpiece.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsenv.h>
#include <iv/lv5/jsscript.h>
#include <iv/lv5/jsfunction.h>
#include <iv/lv5/jsarguments.h>
#include <iv/lv5/railgun/fwd.h>
#include <iv/lv5/railgun/code.h>
#include <iv/lv5/railgun/jsscript.h>
#include <iv/lv5/railgun/context_fwd.h>
#include <iv/lv5/railgun/vm_fwd.h>
#include <iv/lv5/railgun/frame.h>
namespace iv {
namespace lv5 {
namespace railgun {

class JSVMFunction : public JSFunction {
 public:
  JSVal Call(Arguments* args, const JSVal& this_binding, Error* e) {
    args->set_this_binding(this_binding);
    const std::pair<JSVal, VM::State> ret =
        static_cast<Context*>(args->ctx())->vm()->Execute(*args, this, e);
    if (ret.second == VM::STATE_NORMAL) {
      return JSUndefined;
    } else {
      return ret.first;
    }
  }

  JSVal Construct(Arguments* args, Error* e) {
    Context* const ctx = static_cast<Context*>(args->ctx());
    JSObject* const obj = JSObject::New(ctx, code_->ConstructMap(ctx));
    const JSVal proto = Get(ctx, symbol::prototype(), IV_LV5_ERROR(e));
    if (proto.IsObject()) {
      obj->set_prototype(proto.object());
    }
    assert(args->IsConstructorCalled());
    return Call(args, obj, e);
  }

  JSEnv* scope() const {
    return env_;
  }

  Code* code() const {
    return code_;
  }

  static JSVMFunction* New(Context* ctx,
                           railgun::Code* code, JSEnv* env) {
    return new JSVMFunction(ctx, code, env);
  }

  bool IsNativeFunction() const {
    return false;
  }

  bool IsBoundFunction() const {
    return false;
  }

  core::UStringPiece GetSource() const {
    const std::size_t start_pos = code_->block_begin_position();
    const std::size_t end_pos = code_->block_end_position();
    return code_->script()->SubString(start_pos, end_pos - start_pos);
  }

  bool IsStrict() const {
    return code_->strict();
  }

  void MarkChildren(radio::Core* core) {
    core->MarkCell(code_);
    core->MarkCell(env_);
  }

 private:
  JSVMFunction(Context* ctx,
               railgun::Code* code, JSEnv* env)
    : JSFunction(ctx),
      code_(code),
      env_(env) {
    Error e;
    DefineOwnProperty(
        ctx, symbol::length(),
        DataDescriptor(
            JSVal::UInt32(static_cast<uint32_t>(code->params().size())),
            ATTR::NONE),
        false, &e);
    // section 13.2 Creating Function Objects
    set_cls(JSFunction::GetClass());
    set_prototype(context::GetClassSlot(ctx, Class::Function).prototype);

    JSObject* const proto = JSObject::New(ctx);
    proto->DefineOwnProperty(
        ctx, symbol::constructor(),
        DataDescriptor(this, ATTR::W | ATTR::C),
        false, &e);
    DefineOwnProperty(
        ctx, symbol::prototype(),
        DataDescriptor(proto, ATTR::W),
        false, &e);
    if (code->HasName()) {
      DefineOwnProperty(
          ctx, context::Intern(ctx, "name"),
          DataDescriptor(JSString::New(ctx, code->name()), ATTR::NONE),
          false, &e);
    }
    if (code->strict()) {
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

  Code* code_;
  JSEnv* env_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_JSFUNCTION_H_
